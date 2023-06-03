// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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

#include "ModrinthPackExportTask.h"

#include <QCryptographicHash>
#include <QFileInfo>
#include <QMessageBox>
#include <QtConcurrentRun>
#include "Json.h"
#include "MMCZip.h"
#include "minecraft/PackProfile.h"
#include "minecraft/mod/ModFolderModel.h"

const QStringList ModrinthPackExportTask::PREFIXES({ "mods", "coremods", "resourcepacks", "texturepacks", "shaderpacks" });
const QStringList ModrinthPackExportTask::FILE_EXTENSIONS({ "jar", "litemod", "zip" });

ModrinthPackExportTask::ModrinthPackExportTask(const QString& name,
                                               const QString& version,
                                               const QString& summary,
                                               InstancePtr instance,
                                               const QString& output,
                                               MMCZip::FilterFunction filter)
    : name(name)
    , version(version)
    , summary(summary)
    , instance(instance)
    , mcInstance(dynamic_cast<MinecraftInstance*>(instance.get()))
    , gameRoot(instance->gameRoot())
    , output(output)
    , filter(filter)
{}

void ModrinthPackExportTask::executeTask()
{
    setStatus(tr("Searching for files..."));
    setProgress(0, 0);
    collectFiles();
}

bool ModrinthPackExportTask::abort()
{
    if (task != nullptr) {
        task->abort();
        task = nullptr;
        emitAborted();
        return true;
    }

    if (buildZipFuture.isRunning()) {
        buildZipFuture.cancel();
        // NOTE: Here we don't do `emitAborted()` because it will be done when `buildZipFuture` actually cancels, which may not occur immediately.
        return true;
    }

    return false;
}

void ModrinthPackExportTask::collectFiles()
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

    if (mcInstance) {
        mcInstance->loaderModList()->update();
        connect(mcInstance->loaderModList().get(), &ModFolderModel::updateFinished, this, &ModrinthPackExportTask::collectHashes);
    } else
        collectHashes();
}

void ModrinthPackExportTask::collectHashes()
{
    for (const QFileInfo& file : files) {
        QCoreApplication::processEvents();

        const QString relative = gameRoot.relativeFilePath(file.absoluteFilePath());
        // require sensible file types
        if (!std::any_of(PREFIXES.begin(), PREFIXES.end(),
                         [&relative](const QString& prefix) { return relative.startsWith(prefix + QDir::separator()); }))
            continue;
        if (!std::any_of(FILE_EXTENSIONS.begin(), FILE_EXTENSIONS.end(), [&relative](const QString& extension) {
                return relative.endsWith('.' + extension) || relative.endsWith('.' + extension + ".disabled");
            })) {
            continue;
        }

        QCryptographicHash sha512(QCryptographicHash::Algorithm::Sha512);

        QFile openFile(file.absoluteFilePath());
        if (!openFile.open(QFile::ReadOnly)) {
            qWarning() << "Could not open" << file << "for hashing";
            continue;
        }

        const QByteArray data = openFile.readAll();
        if (openFile.error() != QFileDevice::NoError) {
            qWarning() << "Could not read" << file;
            continue;
        }
        sha512.addData(data);

        auto allMods = mcInstance->loaderModList()->allMods();
        if (auto modIter = std::find_if(allMods.begin(), allMods.end(), [&file](Mod* mod) { return mod->fileinfo() == file; });
            modIter != allMods.end()) {
            const Mod* mod = *modIter;
            if (mod->metadata() != nullptr) {
                QUrl& url = mod->metadata()->url;
                // ensure the url is permitted on modrinth.com
                if (!url.isEmpty() && BuildConfig.MODRINTH_MRPACK_HOSTS.contains(url.host())) {
                    qDebug() << "Resolving" << relative << "from index";

                    QCryptographicHash sha1(QCryptographicHash::Algorithm::Sha1);
                    sha1.addData(data);

                    ResolvedFile file{ sha1.result().toHex(), sha512.result().toHex(), url.toString(), openFile.size() };
                    resolvedFiles[relative] = file;

                    // nice! we've managed to resolve based on local metadata!
                    // no need to enqueue it
                    continue;
                }
            }
        }

        qDebug() << "Enqueueing" << relative << "for Modrinth query";
        pendingHashes[relative] = sha512.result().toHex();
    }

    setAbortable(true);
    makeApiRequest();
}

void ModrinthPackExportTask::makeApiRequest()
{
    if (pendingHashes.isEmpty())
        buildZip();
    else {
        QByteArray* response = new QByteArray;
        task = api.currentVersions(pendingHashes.values(), "sha512", response);
        connect(task.get(), &NetJob::succeeded, [this, response]() { parseApiResponse(response); });
        connect(task.get(), &NetJob::failed, this, &ModrinthPackExportTask::emitFailed);
        task->start();
    }
}

void ModrinthPackExportTask::parseApiResponse(const QByteArray* response)
{
    task = nullptr;

    try {
        const QJsonDocument doc = Json::requireDocument(*response);

        QMapIterator<QString, QString> iterator(pendingHashes);
        while (iterator.hasNext()) {
            iterator.next();

            const QJsonObject obj = doc[iterator.value()].toObject();
            if (obj.isEmpty())
                continue;

            const QJsonArray files = obj["files"].toArray();
            if (auto fileIter = std::find_if(files.begin(), files.end(),
                                             [&iterator](const QJsonValue& file) { return file["hashes"]["sha512"] == iterator.value(); });
                fileIter != files.end()) {
                // map the file to the url
                resolvedFiles[iterator.key()] =
                    ResolvedFile{ fileIter->toObject()["hashes"].toObject()["sha1"].toString(), iterator.value(),
                                  fileIter->toObject()["url"].toString(), fileIter->toObject()["size"].toInt() };
            }
        }
    } catch (const Json::JsonException& e) {
        emitFailed(tr("Failed to parse versions response: %1").arg(e.what()));
        return;
    }
    pendingHashes.clear();
    buildZip();
}

void ModrinthPackExportTask::buildZip()
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
        if (!indexFile.open(QIODevice::WriteOnly, QuaZipNewInfo("modrinth.index.json"))) {
            QFile::remove(output);
            return BuildZipResult(tr("Could not create index"));
        }
        indexFile.write(generateIndex());

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
    connect(&buildZipWatcher, &QFutureWatcher<BuildZipResult>::finished, this, &ModrinthPackExportTask::finish);
    buildZipWatcher.setFuture(buildZipFuture);
}

void ModrinthPackExportTask::finish()
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

QByteArray ModrinthPackExportTask::generateIndex()
{
    QJsonObject obj;
    obj["formatVersion"] = 1;
    obj["game"] = "minecraft";
    obj["name"] = name;
    obj["versionId"] = version;
    if (!summary.isEmpty())
        obj["summary"] = summary;

    if (mcInstance) {
        auto profile = mcInstance->getPackProfile();
        // collect all supported components
        const ComponentPtr minecraft = profile->getComponent("net.minecraft");
        const ComponentPtr quilt = profile->getComponent("org.quiltmc.quilt-loader");
        const ComponentPtr fabric = profile->getComponent("net.fabricmc.fabric-loader");
        const ComponentPtr forge = profile->getComponent("net.minecraftforge");

        // convert all available components to mrpack dependencies
        QJsonObject dependencies;
        if (minecraft != nullptr)
            dependencies["minecraft"] = minecraft->m_version;
        if (quilt != nullptr)
            dependencies["quilt-loader"] = quilt->m_version;
        if (fabric != nullptr)
            dependencies["fabric-loader"] = fabric->m_version;
        if (forge != nullptr)
            dependencies["forge"] = forge->m_version;

        obj["dependencies"] = dependencies;
    }

    QJsonArray files;
    QMapIterator<QString, ResolvedFile> iterator(resolvedFiles);
    while (iterator.hasNext()) {
        iterator.next();

        const ResolvedFile& value = iterator.value();

        QJsonObject file;
        QString path = iterator.key();
        path.replace(QDir::separator(), "/");
        file["path"] = path;
        file["downloads"] = QJsonArray({ iterator.value().url });

        QJsonObject hashes;
        hashes["sha1"] = value.sha1;
        hashes["sha512"] = value.sha512;

        file["hashes"] = hashes;
        file["fileSize"] = value.size;

        files << file;
    }
    obj["files"] = files;

    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}
