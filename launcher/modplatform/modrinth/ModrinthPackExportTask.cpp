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
    , mcInstance(dynamic_cast<const MinecraftInstance*>(instance.get()))
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
        if (!task->abort())
            return false;

        task = nullptr;
        emitAborted();
        return true;
    }

    pendingAbort = true;
    return true;
}

void ModrinthPackExportTask::collectFiles()
{
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
    QDir mc(instance->gameRoot());
    for (QFileInfo file : files) {
        QString relative = mc.relativeFilePath(file.absoluteFilePath());
        // require sensible file types
        if (!(relative.endsWith(".zip") || relative.endsWith(".jar") || relative.endsWith(".litemod")))
            continue;

        if (!std::any_of(PREFIXES.begin(), PREFIXES.end(),
                         [&relative](const QString& prefix) { return relative.startsWith(prefix + QDir::separator()); }))
            continue;

        QCryptographicHash hash(QCryptographicHash::Algorithm::Sha512);

        QFile openFile(file.absoluteFilePath());
        if (!openFile.open(QFile::ReadOnly)) {
            qWarning() << "Could not open" << file << "for hashing";
            continue;
        }

        QByteArray data = openFile.readAll();
        if (openFile.error() != QFileDevice::NoError) {
            qWarning() << "Could not read" << file;
            continue;
        }
        hash.addData(data);

        auto allMods = mcInstance->loaderModList()->allMods();
        if (auto modIter = std::find_if(allMods.begin(), allMods.end(), [&file](Mod* mod) { return mod->fileinfo() == file; });
            modIter != allMods.end()) {
            Mod* mod = *modIter;
            if (mod->metadata() != nullptr) {
                QUrl& url = mod->metadata()->url;
                // most likely some of these may be from curseforge
                if (!url.isEmpty() && BuildConfig.MODRINTH_MRPACK_HOSTS.contains(url.host())) {
                    qDebug() << "Resolving" << relative << "from index";

                    QCryptographicHash hash2(QCryptographicHash::Algorithm::Sha1);
                    hash2.addData(data);

                    ResolvedFile file{ hash2.result().toHex(), hash.result().toHex(), url.toString(), openFile.size() };
                    resolvedFiles[relative] = file;

                    // nice! we've managed to resolve based on local metadata!
                    // no need to enqueue it
                    continue;
                }
            }
        }

        qDebug() << "Enqueueing" << relative << "for Modrinth query";
        pendingHashes[relative] = hash.result().toHex();
    }

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

void ModrinthPackExportTask::parseApiResponse(QByteArray* response)
{
    task = nullptr;

    try {
        const QJsonDocument doc = Json::requireDocument(*response);

        QMapIterator<QString, QString> iterator(pendingHashes);
        while (iterator.hasNext()) {
            iterator.next();

            QJsonObject obj = doc[iterator.value()].toObject();
            if (obj.isEmpty())
                continue;

            QJsonArray files = obj["files"].toArray();
            if (auto fileIter = std::find_if(files.begin(), files.end(),
                                             [&iterator](const QJsonValue& file) { return file["hashes"]["sha512"] == iterator.value(); });
                fileIter != files.end()) {
                // map the file to the url
                resolvedFiles[iterator.key()] =
                    ResolvedFile{ fileIter->toObject()["hashes"].toObject()["sha1"].toString(), iterator.value(),
                                  fileIter->toObject()["url"].toString(), fileIter->toObject()["size"].toInt() };
            }
        }
    } catch (Json::JsonException& e) {
        qWarning() << "Failed to parse versions response" << e.what();
    }
    pendingHashes.clear();
    buildZip();
}

void ModrinthPackExportTask::buildZip()
{
    static_cast<void>(QtConcurrent::run(QThreadPool::globalInstance(), [this]() {
        setStatus("Adding files...");
        QuaZip zip(output);
        if (!zip.open(QuaZip::mdCreate)) {
            QFile::remove(output);
            emitFailed(tr("Could not create file"));
            return;
        }

        if (pendingAbort) {
            QMetaObject::invokeMethod(this, &ModrinthPackExportTask::emitAborted, Qt::QueuedConnection);
            return;
        }

        QuaZipFile indexFile(&zip);
        if (!indexFile.open(QIODevice::WriteOnly, QuaZipNewInfo("modrinth.index.json"))) {
            QFile::remove(output);
            QMetaObject::invokeMethod(
                this, [this]() { emitFailed(tr("Could not create index")); }, Qt::QueuedConnection);
            return;
        }
        indexFile.write(generateIndex());

        QDir mc(instance->gameRoot());
        size_t i = 0;
        for (const QFileInfo& file : files) {
            if (pendingAbort) {
                QFile::remove(output);
                QMetaObject::invokeMethod(this, &ModrinthPackExportTask::emitAborted, Qt::QueuedConnection);
                return;
            }

            setProgress(i, files.length());
            QString relative = mc.relativeFilePath(file.absoluteFilePath());
            if (!resolvedFiles.contains(relative) && !JlCompress::compressFile(&zip, file.absoluteFilePath(), "overrides/" + relative)) {
                QFile::remove(output);
                QMetaObject::invokeMethod(
                    this, [this, relative]() { emitFailed(tr("Could not read and compress %1").arg(relative)); }, Qt::QueuedConnection);
                return;
            }
            i++;
        }

        zip.close();

        if (zip.getZipError() != 0) {
            QFile::remove(output);
            QMetaObject::invokeMethod(
                this, [this]() { emitFailed(tr("A zip error occurred")); }, Qt::QueuedConnection);
            return;
        }

        QMetaObject::invokeMethod(this, &ModrinthPackExportTask::emitSucceeded, Qt::QueuedConnection);
    }));
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

    MinecraftInstance* mc = dynamic_cast<MinecraftInstance*>(instance.get());
    if (mc) {
        auto profile = mc->getPackProfile();
        // collect all supported components
        ComponentPtr minecraft = profile->getComponent("net.minecraft");
        ComponentPtr quilt = profile->getComponent("org.quiltmc.quilt-loader");
        ComponentPtr fabric = profile->getComponent("net.fabricmc.fabric-loader");
        ComponentPtr forge = profile->getComponent("net.minecraftforge");

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
