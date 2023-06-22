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
#include "Json.h"
#include "MMCZip.h"
#include "minecraft/PackProfile.h"
#include "minecraft/mod/ModFolderModel.h"
#include "modplatform/helpers/ExportModsToStringTask.h"

const QStringList FlamePackExportTask::PREFIXES({ "mods/", "coremods/", "resourcepacks/", "texturepacks/", "shaderpacks/" });
const QStringList FlamePackExportTask::FILE_EXTENSIONS({ "jar", "litemod", "zip" });
const QString FlamePackExportTask::TEMPLATE = "<li><a href={url}>{name}({authors})</a></li>";

FlamePackExportTask::FlamePackExportTask(const QString& name,
                                         const QString& version,
                                         const QVariant& projectID,
                                         InstancePtr instance,
                                         const QString& output,
                                         MMCZip::FilterFunction filter)
    : name(name)
    , version(version)
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

    resolvedFiles.clear();

    mcInstance->loaderModList()->update();
    connect(mcInstance->loaderModList().get(), &ModFolderModel::updateFinished, this, [this]() {
        mods = mcInstance->loaderModList()->allMods();
        buildZip();
    });
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
        QString content = ExportToString::ExportModsToStringTask(mods, TEMPLATE);
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
            if (!resolvedFiles.contains(relative) && !JlCompress::compressFile(&zip, file.absoluteFilePath(), "overrides/" + relative)) {
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
            loader["id"] = quilt->getName();
        else if (fabric != nullptr)
            loader["id"] = fabric->getName();
        else if (forge != nullptr)
            loader["id"] = forge->getName();
        loader["primary"] = true;

        version["modLoaders"] = QJsonArray({ loader });
        obj["minecraft"] = version;
    }

    QJsonArray files;
    QMapIterator<QString, ResolvedFile> iterator(resolvedFiles);
    while (iterator.hasNext()) {
        iterator.next();

        const ResolvedFile& value = iterator.value();

        QJsonObject file;
        file["projectID"] = value.projectID.toInt();
        file["fileID"] = value.fileID.toInt();
        file["required"] = value.required;
        files << file;
    }
    obj["files"] = files;

    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}
