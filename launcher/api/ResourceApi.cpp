// SPDX-License-Identifier: GPL-3.0-only
/*
 *  MaterialMC - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ResourceApi.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QJsonArray>
#include <QTimer>

#include <functional>

#include "Application.h"
#include "FileSystem.h"
#include "GZip.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/World.h"
#include "minecraft/WorldList.h"
#include "minecraft/mod/ModFolderModel.h"
#include "minecraft/mod/Resource.h"
#include "minecraft/mod/ResourceFolderModel.h"
#include "minecraft/mod/ResourcePackFolderModel.h"
#include "minecraft/mod/ShaderPackFolderModel.h"
#include "minecraft/mod/TexturePackFolderModel.h"

#include "ApiRouter.h"
#include "ApiUtils.h"
#include "TaskTracker.h"

namespace api {

namespace {

constexpr qint64 MaxLogBytes = 4 * 1024 * 1024;

QJsonObject serializeResource(const Resource& r)
{
    QJsonArray issues;
    for (const auto& issue : r.issues()) {
        issues.append(issue);
    }
    return {
        { "id", r.internalId() },
        { "name", r.name() },
        { "fileName", r.fileinfo().fileName() },
        { "version", r.version() },
        { "enabled", r.enabled() },
        { "provider", r.metadata() ? QJsonValue(r.provider()) : QJsonValue() },
        { "sizeBytes", static_cast<double>(r.sizeInfo()) },
        { "modified", timestamp(r.dateTimeChanged()) },
        { "homepage", r.homepage() },
        { "issues", issues },
    };
}

QModelIndexList indexesFor(ResourceFolderModel* model, const QStringList& ids)
{
    QModelIndexList out;
    for (const auto& id : ids) {
        bool found = false;
        for (int row = 0; row < model->size(); row++) {
            if (model->at(row).internalId() == id) {
                out << model->index(row, 0);
                found = true;
                break;
            }
        }
        if (!found) {
            throw ApiError::notFound(QObject::tr("'%1' is not installed").arg(id));
        }
    }
    return out;
}

int worldIndex(WorldList* worlds, const QString& worldId)
{
    for (qsizetype i = 0; i < worlds->size(); i++) {
        if (worlds->allWorlds().at(i).folderName() == worldId) {
            return static_cast<int>(i);
        }
    }
    throw ApiError::notFound(QObject::tr("World '%1' does not exist").arg(worldId));
}

bool isImage(const QString& name)
{
    const auto lower = name.toLower();
    return lower.endsWith(".png") || lower.endsWith(".jpg") || lower.endsWith(".jpeg");
}

QString screenshotsDir(MinecraftInstance* instance)
{
    return FS::PathCombine(instance->gameRoot(), "screenshots");
}

/** Game log files as `<folder>/<file>` relative names, newest first. */
QList<QFileInfo> gameLogFiles(MinecraftInstance* instance)
{
    QList<QFileInfo> files;
    const QDir logs(FS::PathCombine(instance->gameRoot(), "logs"));
    files << logs.entryInfoList({ "*.log", "*.log.gz", "*.txt" }, QDir::Files);
    const QDir crashes(FS::PathCombine(instance->gameRoot(), "crash-reports"));
    files << crashes.entryInfoList({ "*.txt" }, QDir::Files);
    std::sort(files.begin(), files.end(), [](const QFileInfo& a, const QFileInfo& b) { return a.lastModified() > b.lastModified(); });
    return files;
}

QString logName(const QFileInfo& file)
{
    return file.dir().dirName() + '/' + file.fileName();
}

}  // namespace

ResourceApi::ResourceApi(ApiRouter* router, TaskTracker* tasks, QObject* parent) : QObject(parent), m_router(router), m_tasks(tasks)
{
    registerMethods();
}

ResourceFolderModel* ResourceApi::modelFor(MinecraftInstance* instance, const QString& kind)
{
    if (kind == QLatin1String("mods")) {
        return instance->loaderModList();
    }
    if (kind == QLatin1String("resourcepacks")) {
        return instance->resourcePackList();
    }
    if (kind == QLatin1String("shaderpacks")) {
        return instance->shaderPackList();
    }
    if (kind == QLatin1String("texturepacks")) {
        return instance->texturePackList();
    }
    throw ApiError::invalidParams(QObject::tr("Unknown resource kind '%1'").arg(kind));
}

QString ResourceApi::screenshotPath(MinecraftInstance* instance, const QString& name)
{
    if (name.isEmpty() || name.contains('/') || name.contains('\\') || name.startsWith('.') || !isImage(name)) {
        return {};
    }
    const QFileInfo file(FS::PathCombine(screenshotsDir(instance), name));
    return file.isFile() ? file.absoluteFilePath() : QString();
}

QString ResourceApi::worldIconPath(MinecraftInstance* instance, const QString& worldId)
{
    for (const auto& world : instance->worldList()->allWorlds()) {
        if (world.folderName() == worldId) {
            return world.iconFile();
        }
    }
    return {};
}

void ResourceApi::ensureLoaded(MinecraftInstance* instance, const QString& kind, ResourceFolderModel* model, std::function<void()> then)
{
    if (m_loadedModels.contains(model)) {
        then();
        return;
    }
    const auto instanceId = instance->id();
    m_loadedModels.insert(model);
    connect(model, &QObject::destroyed, this, [this, model] { m_loadedModels.remove(model); });
    // onUpdateSucceeded is queued after updateFinished; defer so listeners see the new data.
    connect(model, &ResourceFolderModel::updateFinished, this, [this, instanceId, kind] {
        QTimer::singleShot(0, this, [this, instanceId, kind] {
            m_router->emitEvent("resources.changed", QJsonObject{ { "instanceId", instanceId }, { "kind", kind } });
        });
    });
    connect(model, &ResourceFolderModel::updateFinished, this, [this, then] { QTimer::singleShot(0, this, then); }, Qt::SingleShotConnection);
    model->startWatching();
    model->update();
}

void ResourceApi::ensureWorldsLoaded(MinecraftInstance* instance, WorldList* worlds)
{
    if (m_loadedWorlds.contains(worlds)) {
        return;
    }
    m_loadedWorlds.insert(worlds);
    const auto instanceId = instance->id();
    connect(worlds, &QObject::destroyed, this, [this, worlds] { m_loadedWorlds.remove(worlds); });
    // Metadata (names, sizes) arrives asynchronously row by row; coalesce the notifications.
    auto* timer = new QTimer(worlds);
    timer->setSingleShot(true);
    timer->setInterval(200);
    connect(timer, &QTimer::timeout, this, [this, instanceId] { m_router->emitEvent("worlds.changed", QJsonObject{ { "instanceId", instanceId } }); });
    auto schedule = [timer] { timer->start(); };
    connect(worlds, &QAbstractItemModel::dataChanged, timer, schedule);
    connect(worlds, &QAbstractItemModel::modelReset, timer, schedule);
    worlds->startWatching();
    worlds->update();
}

void ResourceApi::registerMethods()
{
    m_router->add("resources.list", [this](const QJsonObject& p, const ApiReply& reply) {
        auto* instance = requireInstance(p, "instanceId");
        const auto kind = params::requireNonEmpty(p, "kind", 32);
        auto* model = modelFor(instance, kind);
        ensureLoaded(instance, kind, model, [model, reply] {
            QJsonArray out;
            for (auto* resource : model->allResources()) {
                out.append(serializeResource(*resource));
            }
            reply.resolve(out);
        });
    });

    m_router->addSync("resources.setEnabled", [](const QJsonObject& p) {
        auto* instance = requireInstance(p, "instanceId");
        if (instance->isRunning()) {
            throw ApiError("INSTANCE_RUNNING", QObject::tr("Stop the game before changing its files"));
        }
        auto* model = modelFor(instance, params::requireNonEmpty(p, "kind", 32));
        const auto indexes = indexesFor(model, params::requireStringList(p, "ids"));
        const auto action = params::requireBool(p, "enabled") ? EnableAction::ENABLE : EnableAction::DISABLE;
        if (!model->setResourceEnabled(indexes, action)) {
            throw ApiError::io(QObject::tr("Some files could not be renamed"));
        }
        return ok();
    });

    m_router->addSync("resources.remove", [](const QJsonObject& p) {
        auto* instance = requireInstance(p, "instanceId");
        if (instance->isRunning()) {
            throw ApiError("INSTANCE_RUNNING", QObject::tr("Stop the game before changing its files"));
        }
        auto* model = modelFor(instance, params::requireNonEmpty(p, "kind", 32));
        const auto indexes = indexesFor(model, params::requireStringList(p, "ids"));
        if (!model->deleteResources(indexes)) {
            throw ApiError::io(QObject::tr("Some files could not be deleted"));
        }
        model->update();
        return ok();
    });

    m_router->addSync("resources.importFiles", [](const QJsonObject& p) {
        auto* instance = requireInstance(p, "instanceId");
        const auto kind = params::requireNonEmpty(p, "kind", 32);
        auto* model = modelFor(instance, kind);
        const auto filter = kind == QLatin1String("mods") ? QObject::tr("Mods (*.jar *.zip *.litemod);;All files (*)")
                                                          : QObject::tr("Packs (*.zip);;All files (*)");
        // Native picker: the page never sees or supplies file system paths.
        const auto files = QFileDialog::getOpenFileNames(nullptr, QObject::tr("Add files to %1").arg(instance->name()),
                                                         APPLICATION->settings()->get("DownloadsDir").toString(), filter);
        int imported = 0;
        for (const auto& file : files) {
            if (model->installResource(file)) {
                imported++;
            }
        }
        if (imported > 0) {
            model->update();
        }
        return QJsonObject{ { "imported", imported } };
    });

    m_router->addSync("worlds.list", [this](const QJsonObject& p) {
        auto* instance = requireInstance(p, "instanceId");
        auto* worlds = instance->worldList();
        ensureWorldsLoaded(instance, worlds);
        QJsonArray out;
        for (const auto& w : worlds->allWorlds()) {
            const auto icon = w.iconFile();
            out.append(QJsonObject{
                { "id", w.folderName() },
                { "name", w.name().isEmpty() ? w.folderName() : w.name() },
                { "gameType", w.gameType().toTranslatedString() },
                { "lastPlayed", timestamp(w.lastPlayed()) },
                { "sizeBytes", static_cast<double>(w.bytes()) },
                { "seed", QString::number(w.seed()) },
                { "iconUrl", icon.isEmpty() || !QFileInfo::exists(icon) ? QJsonValue()
                                                                        : QJsonValue(hostResourceUrl({ "_world", instance->id(), w.folderName() })) },
                { "isValid", w.isValid() },
            });
        }
        return out;
    });

    m_router->addSync("worlds.remove", [this](const QJsonObject& p) {
        auto* instance = requireInstance(p, "instanceId");
        if (instance->isRunning()) {
            throw ApiError("INSTANCE_RUNNING", QObject::tr("Stop the game before deleting worlds"));
        }
        auto* worlds = instance->worldList();
        const auto worldId = params::requireFileName(p, "worldId");
        auto task = worlds->createDeleteWorldTask(worldIndex(worlds, worldId));
        if (!task) {
            throw ApiError::io(QObject::tr("The world cannot be deleted"));
        }
        const Task::Ptr shared(task.release());
        connect(shared.get(), &Task::finished, worlds, [worlds] { worlds->update(); });
        return QJsonObject{ { "taskId", m_tasks->start(shared, "world.delete", QObject::tr("Deleting world %1").arg(worldId), instance->id()) } };
    });

    m_router->addSync("worlds.rename", [](const QJsonObject& p) {
        auto* instance = requireInstance(p, "instanceId");
        if (instance->isRunning()) {
            throw ApiError("INSTANCE_RUNNING", QObject::tr("Stop the game before renaming worlds"));
        }
        auto* worlds = instance->worldList();
        const auto index = worldIndex(worlds, params::requireFileName(p, "worldId"));
        if (!(*worlds)[index].rename(params::requireNonEmpty(p, "name", 128))) {
            throw ApiError::io(QObject::tr("The world could not be renamed"));
        }
        worlds->update();
        return ok();
    });

    m_router->addSync("screenshots.list", [](const QJsonObject& p) {
        auto* instance = requireInstance(p, "instanceId");
        QDir dir(screenshotsDir(instance));
        const auto files = dir.entryInfoList({ "*.png", "*.jpg", "*.jpeg" }, QDir::Files, QDir::Time);
        QJsonArray out;
        for (const auto& f : files) {
            out.append(QJsonObject{
                { "name", f.fileName() },
                { "url", hostResourceUrl({ "_screenshot", instance->id(), f.fileName() }) },
                { "modified", timestamp(f.lastModified()) },
                { "sizeBytes", static_cast<double>(f.size()) },
            });
        }
        return out;
    });

    m_router->addSync("screenshots.remove", [](const QJsonObject& p) {
        auto* instance = requireInstance(p, "instanceId");
        const auto names = params::requireStringList(p, "names", 1000);
        QStringList failed;
        for (const auto& name : names) {
            const auto path = screenshotPath(instance, name);
            if (path.isEmpty()) {
                throw ApiError::notFound(QObject::tr("Screenshot '%1' does not exist").arg(name));
            }
            if (!FS::trash(path) && !QFile::remove(path)) {
                failed << name;
            }
        }
        if (!failed.isEmpty()) {
            throw ApiError::io(QObject::tr("Could not delete: %1").arg(failed.join(", ")));
        }
        return ok();
    });

    m_router->addSync("logs.list", [](const QJsonObject& p) {
        auto* instance = requireInstance(p, "instanceId");
        QJsonArray out;
        for (const auto& f : gameLogFiles(instance)) {
            out.append(QJsonObject{ { "name", logName(f) }, { "sizeBytes", static_cast<double>(f.size()) }, { "modified", timestamp(f.lastModified()) } });
        }
        return out;
    });

    m_router->addSync("logs.read", [](const QJsonObject& p) {
        auto* instance = requireInstance(p, "instanceId");
        const auto name = params::requireNonEmpty(p, "name", 512);
        // Only files that logs.list would return can be read.
        const auto files = gameLogFiles(instance);
        const auto it = std::find_if(files.begin(), files.end(), [&](const QFileInfo& f) { return logName(f) == name; });
        if (it == files.end()) {
            throw ApiError::notFound(QObject::tr("Log file '%1' does not exist").arg(name));
        }
        QFile file(it->absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly)) {
            throw ApiError::io(file.errorString());
        }
        QByteArray data;
        if (name.endsWith(".gz")) {
            if (!GZip::unzip(file.readAll(), data)) {
                throw ApiError::io(QObject::tr("Could not decompress %1").arg(name));
            }
        } else {
            if (file.size() > MaxLogBytes) {
                file.seek(file.size() - MaxLogBytes);
            }
            data = file.readAll();
        }
        bool truncated = false;
        if (data.size() > MaxLogBytes || (file.size() > MaxLogBytes && !name.endsWith(".gz"))) {
            data = data.right(MaxLogBytes);
            const auto newline = data.indexOf('\n');
            if (newline >= 0) {
                data = data.mid(newline + 1);
            }
            truncated = true;
        }
        return QJsonObject{ { "name", name }, { "content", QString::fromUtf8(data) }, { "truncated", truncated } };
    });
}

}  // namespace api
