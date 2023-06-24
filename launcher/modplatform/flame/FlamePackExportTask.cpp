// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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

#include "FlamePackExportTask.h"
#include <QJsonArray>
#include <QJsonObject>

#include <QCryptographicHash>
#include <QFileInfo>
#include <QMessageBox>
#include <QtConcurrentRun>
#include <algorithm>
#include <memory>
#include "Json.h"
#include "MMCZip.h"
#include "minecraft/PackProfile.h"
#include "minecraft/mod/ModFolderModel.h"
#include "modplatform/ModIndex.h"
#include "modplatform/flame/FlameModIndex.h"
#include "modplatform/helpers/HashUtils.h"
#include "tasks/Task.h"

const QString FlamePackExportTask::TEMPLATE = "<li><a href={url}>{name} (by {authors})</a></li>";

FlamePackExportTask::FlamePackExportTask(const QString& name,
                                         const QString& version,
                                         const QString& author,
                                         const QVariant& projectID,
                                         InstancePtr instance,
                                         const QString& output,
                                         MMCZip::FilterFunction filter)
    : name(name)
    , version(version)
    , author(author)
    , projectID(projectID)
    , instance(instance)
    , mcInstance(dynamic_cast<MinecraftInstance*>(instance.get()))
    , gameRoot(instance->gameRoot())
    , output(output)
    , filter(filter)
{}

void FlamePackExportTask::executeTask()
{
    setStatus(tr("Searching for files..."));
    setProgress(0, 0);
    collectFiles();
}

bool FlamePackExportTask::abort()
{
    if (task != nullptr) {
        task->abort();
        task = nullptr;
        emitAborted();
        return true;
    }

    if (buildZipFuture.isRunning()) {
        buildZipFuture.cancel();
        // NOTE: Here we don't do `emitAborted()` because it will be done when `buildZipFuture` actually cancels, which may not occur
        // immediately.
        return true;
    }

    return false;
}

void FlamePackExportTask::collectFiles()
{
    setAbortable(false);
    QCoreApplication::processEvents();

    files.clear();
    if (!MMCZip::collectFileListRecursively(instance->gameRoot(), nullptr, &files, filter)) {
        emitFailed(tr("Could not search for files"));
        return;
    }

    pendingHashes.clear();
    resolvedFiles.clear();

    if (mcInstance != nullptr) {
        connect(mcInstance->loaderModList().get(), &ModFolderModel::updateFinished, this, &FlamePackExportTask::collectHashes);
        mcInstance->loaderModList()->update();
    } else
        collectHashes();
}

void FlamePackExportTask::collectHashes()
{
    setAbortable(true);
    setStatus(tr("Find file hashes..."));
    auto mods = mcInstance->loaderModList()->allMods();
    ConcurrentTask::Ptr hashing_task(new ConcurrentTask(this, "MakeHashesTask", 10));
    task.reset(hashing_task);
    setProgress(0, mods.count());
    for (auto* mod : mods) {
        if (!mod || mod->type() == ResourceType::FOLDER) {
            setProgress(m_progress + 1, mods.count());
            continue;
        }
        if (mod->metadata() && mod->metadata()->provider == ModPlatform::ResourceProvider::FLAME) {
            resolvedFiles.insert(mod->fileinfo().absoluteFilePath(),
                                 { mod->metadata()->project_id.toInt(), mod->metadata()->file_id.toInt(), mod->enabled(),
                                   mod->metadata()->name, mod->metadata()->slug, mod->authors().join(", ") });
            setProgress(m_progress + 1, mods.count());
            continue;
        }

        auto hash_task = Hashing::createFlameHasher(mod->fileinfo().absoluteFilePath());
        connect(hash_task.get(), &Hashing::Hasher::resultsReady, [this, mod, mods](QString hash) {
            if (m_state == Task::State::Running) {
                setProgress(m_progress + 1, mods.count());
                pendingHashes.insert(hash, mod);
            }
        });
        connect(hash_task.get(), &Task::failed, this, &FlamePackExportTask::emitFailed);
        hashing_task->addTask(hash_task);
    }
    connect(hashing_task.get(), &Task::succeeded, this, &FlamePackExportTask::makeApiRequest);
    connect(hashing_task.get(), &Task::failed, this, &FlamePackExportTask::emitFailed);
    hashing_task->start();
}

void FlamePackExportTask::makeApiRequest()
{
    setStatus(tr("Find versions for hashes..."));
    if (pendingHashes.isEmpty()) {
        buildZip();
        return;
    }

    auto response = std::make_shared<QByteArray>();

    QList<uint> fingerprints;
    for (auto& murmur : pendingHashes.keys()) {
        fingerprints.push_back(murmur.toUInt());
    }

    task.reset(api.matchFingerprints(fingerprints, response));

    connect(task.get(), &Task::succeeded, this, [this, response] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from Modrinth::CurrentVersions at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;

            failed(parse_error.errorString());
            return;
        }

        try {
            auto doc_obj = Json::requireObject(doc);
            auto data_obj = Json::requireObject(doc_obj, "data");
            auto data_arr = Json::requireArray(data_obj, "exactMatches");

            if (data_arr.isEmpty()) {
                qWarning() << "No matches found for fingerprint search!";

                return;
            }
            size_t progress = 0;
            for (auto match : data_arr) {
                setProgress(progress++, data_arr.count());
                auto match_obj = Json::ensureObject(match, {});
                auto file_obj = Json::ensureObject(match_obj, "file", {});

                if (match_obj.isEmpty() || file_obj.isEmpty()) {
                    qWarning() << "Fingerprint match is empty!";

                    return;
                }

                auto fingerprint = QString::number(Json::ensureVariant(file_obj, "fileFingerprint").toUInt());
                auto mod = pendingHashes.find(fingerprint);
                if (mod == pendingHashes.end()) {
                    qWarning() << "Invalid fingerprint from the API response.";
                    continue;
                }

                setStatus(tr("Parsing API response from CurseForge for '%1'...").arg((*mod)->name()));
                if (Json::ensureBoolean(file_obj, "isAvailable", false))
                    resolvedFiles.insert(
                        mod.value()->fileinfo().absoluteFilePath(),
                        { Json::requireInteger(file_obj, "modId"), Json::requireInteger(file_obj, "id"), mod.value()->enabled() });
            }

        } catch (Json::JsonException& e) {
            qDebug() << e.cause();
            qDebug() << doc;
        }
        pendingHashes.clear();
    });
    connect(task.get(), &Task::finished, this, &FlamePackExportTask::getProjectsInfo);
    connect(task.get(), &NetJob::failed, this, &FlamePackExportTask::emitFailed);
    task->start();
}

void FlamePackExportTask::getProjectsInfo()
{
    setStatus(tr("Find project info from curseforge..."));
    QList<QString> addonIds;
    for (auto resolved : resolvedFiles) {
        if (resolved.slug.isEmpty()) {
            addonIds << QString::number(resolved.addonId);
        }
    }

    auto response = std::make_shared<QByteArray>();
    Task::Ptr proj_task;

    if (addonIds.isEmpty()) {
        buildZip();
        return;
    } else if (addonIds.size() == 1) {
        proj_task = api.getProject(*addonIds.begin(), response);
    } else {
        proj_task = api.getProjects(addonIds, response);
    }

    connect(proj_task.get(), &Task::succeeded, this, [this, response, addonIds] {
        QJsonParseError parse_error{};
        auto doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from Modrinth projects task at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;
            return;
        }

        try {
            QJsonArray entries;
            if (addonIds.size() == 1)
                entries = { Json::requireObject(Json::requireObject(doc), "data") };
            else
                entries = Json::requireArray(Json::requireObject(doc), "data");

            size_t progress = 0;
            for (auto entry : entries) {
                setProgress(progress++, entries.count());
                auto entry_obj = Json::requireObject(entry);

                try {
                    setStatus(tr("Parsing API response from CurseForge for '%1'...").arg(Json::requireString(entry_obj, "name")));

                    ModPlatform::IndexedPack pack;
                    FlameMod::loadIndexedPack(pack, entry_obj);
                    for (auto key : resolvedFiles.keys()) {
                        auto val = resolvedFiles.value(key);
                        if (val.addonId == pack.addonId) {
                            val.name = pack.name;
                            val.slug = pack.slug;
                            QStringList authors;
                            for (auto author : pack.authors)
                                authors << author.name;

                            val.authors = authors.join(", ");
                            resolvedFiles[key] = val;
                        }
                    }

                } catch (Json::JsonException& e) {
                    qDebug() << e.cause();
                    qDebug() << entries;
                }
            }
        } catch (Json::JsonException& e) {
            qDebug() << e.cause();
            qDebug() << doc;
        }
        buildZip();
    });
    task.reset(proj_task);
    task->start();
}

void FlamePackExportTask::buildZip()
{
    setStatus(tr("Adding files..."));

    buildZipFuture = QtConcurrent::run(QThreadPool::globalInstance(), [this]() {
        QuaZip zip(output);
        if (!zip.open(QuaZip::mdCreate)) {
            QFile::remove(output);
            return BuildZipResult(tr("Could not create file"));
        }

        if (buildZipFuture.isCanceled())
            return BuildZipResult();

        QuaZipFile indexFile(&zip);
        if (!indexFile.open(QIODevice::WriteOnly, QuaZipNewInfo("manifest.json"))) {
            QFile::remove(output);
            return BuildZipResult(tr("Could not create index"));
        }
        indexFile.write(generateIndex());

        QuaZipFile modlist(&zip);
        if (!modlist.open(QIODevice::WriteOnly, QuaZipNewInfo("modlist.html"))) {
            QFile::remove(output);
            return BuildZipResult(tr("Could not create index"));
        }
        QString content = "";
        for (auto mod : resolvedFiles) {
            content += QString(TEMPLATE)
                           .replace("{name}", mod.name)
                           .replace("{url}", ModPlatform::getMetaURL(ModPlatform::ResourceProvider::FLAME, mod.slug))
                           .replace("{authors}", mod.authors) +
                       "\n";
        }
        content = "<ul>" + content + "</ul>";
        modlist.write(content.toUtf8());

        size_t progress = 0;
        for (const QFileInfo& file : files) {
            if (buildZipFuture.isCanceled()) {
                QFile::remove(output);
                return BuildZipResult();
            }

            setProgress(progress, files.length());
            const QString relative = gameRoot.relativeFilePath(file.absoluteFilePath());
            if (!resolvedFiles.contains(file.absoluteFilePath()) &&
                !JlCompress::compressFile(&zip, file.absoluteFilePath(), "overrides/" + relative)) {
                QFile::remove(output);
                return BuildZipResult(tr("Could not read and compress %1").arg(relative));
            }
            progress++;
        }

        zip.close();

        if (zip.getZipError() != 0) {
            QFile::remove(output);
            return BuildZipResult(tr("A zip error occurred"));
        }

        return BuildZipResult();
    });
    connect(&buildZipWatcher, &QFutureWatcher<BuildZipResult>::finished, this, &FlamePackExportTask::finish);
    buildZipWatcher.setFuture(buildZipFuture);
}

void FlamePackExportTask::finish()
{
    if (buildZipFuture.isCanceled())
        emitAborted();
    else {
        const BuildZipResult result = buildZipFuture.result();
        if (result.has_value())
            emitFailed(result.value());
        else
            emitSucceeded();
    }
}

QByteArray FlamePackExportTask::generateIndex()
{
    QJsonObject obj;
    obj["manifestType"] = "minecraftModpack";
    obj["manifestVersion"] = 1;
    obj["name"] = name;
    obj["version"] = version;
    obj["author"] = author;
    if (projectID.toInt() != 0)
        obj["projectID"] = projectID.toInt();
    obj["overrides"] = "overrides";
    if (mcInstance) {
        QJsonObject version;
        auto profile = mcInstance->getPackProfile();
        // collect all supported components
        const ComponentPtr minecraft = profile->getComponent("net.minecraft");
        const ComponentPtr quilt = profile->getComponent("org.quiltmc.quilt-loader");
        const ComponentPtr fabric = profile->getComponent("net.fabricmc.fabric-loader");
        const ComponentPtr forge = profile->getComponent("net.minecraftforge");

        // convert all available components to mrpack dependencies
        if (minecraft != nullptr)
            version["version"] = minecraft->m_version;

        QJsonObject loader;
        if (quilt != nullptr)
            loader["id"] = "quilt-" + quilt->getVersion();
        else if (fabric != nullptr)
            loader["id"] = "fabric-" + fabric->getVersion();
        else if (forge != nullptr)
            loader["id"] = "forge-" + forge->getVersion();
        loader["primary"] = true;
        version["modLoaders"] = QJsonArray({ loader });
        obj["minecraft"] = version;
    }

    QJsonArray files;
    for (auto mod : resolvedFiles) {
        QJsonObject file;
        file["projectID"] = mod.addonId;
        file["fileID"] = mod.version;
        file["required"] = mod.enabled;
        files << file;
    }
    obj["files"] = files;

    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}
