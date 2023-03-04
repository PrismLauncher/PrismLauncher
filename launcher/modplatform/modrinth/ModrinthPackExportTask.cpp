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

#include <qcryptographichash.h>
#include <QFileInfo>
#include <QFileInfoList>
#include <QMessageBox>
#include <QtConcurrent>
#include "Json.h"
#include "MMCZip.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "modplatform/modrinth/ModrinthAPI.h"

const QStringList ModrinthPackExportTask::PREFIXES = QStringList({ "mods", "coremods", "resourcepacks", "texturepacks", "shaderpacks" });

ModrinthPackExportTask::ModrinthPackExportTask(const QString& name,
                                               const QString& version,
                                               const QString& summary,
                                               InstancePtr instance,
                                               const QString& output,
                                               MMCZip::FilterFunction filter)
    : name(name), version(version), summary(summary), instance(instance), output(output), filter(filter)
{}

void ModrinthPackExportTask::executeTask()
{
    setStatus(tr("Searching for files..."));
    setProgress(0, 0);
    collectFiles();

    QByteArray* response = new QByteArray;
    task = api.currentVersions(fileHashes.values(), "sha512", response);
    connect(task.get(), &NetJob::succeeded, [this, response]() { parseApiResponse(response); });
    connect(task.get(), &NetJob::failed, this, &ModrinthPackExportTask::emitFailed);
    task->start();
}

bool ModrinthPackExportTask::abort() {
    if (!task.isNull())
        return task->abort();
    return false;
}

void ModrinthPackExportTask::collectFiles()
{
    files.clear();
    if (!MMCZip::collectFileListRecursively(instance->gameRoot(), nullptr, &files, filter)) {
        emitFailed(tr("Could not collect list of files"));
        return;
    }

    fileHashes.clear();

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

        if (!hash.addData(&openFile)) {
            qWarning() << "Could not add hash data for" << file;
            continue;
        }

        fileHashes[relative] = hash.result().toHex();
    }
}

void ModrinthPackExportTask::parseApiResponse(QByteArray* response)
{
    QMap<QString, ResolvedFile> resolved;

    try {
        QJsonDocument doc = Json::requireDocument(*response);

        QMapIterator<QString, QString> iterator(fileHashes);
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
                resolved[iterator.key()] = ResolvedFile{ fileIter->toObject()["hashes"].toObject()["sha1"].toString(), iterator.value(),
                                                         fileIter->toObject()["url"].toString(), fileIter->toObject()["size"].toInt() };
            }
        }
    } catch (Json::JsonException& e) {
        qWarning() << "Failed to parse versions response" << e.what();
    }

    buildZip(resolved);
}

void ModrinthPackExportTask::buildZip(const QMap<QString, ResolvedFile>& resolvedFiles)
{
    setStatus("Adding files...");
    QuaZip zip(output);
    if (!zip.open(QuaZip::mdCreate)) {
        QFile::remove(output);
        emitFailed(tr("Could not create file"));
        return;
    }

    QuaZipFile indexFile(&zip);
    if (!indexFile.open(QIODevice::WriteOnly, QuaZipNewInfo("modrinth.index.json"))) {
        QFile::remove(output);
        emitFailed(tr("Could not create index"));
        return;
    }
    indexFile.write(generateIndex(resolvedFiles));

    QDir mc(instance->gameRoot());
    size_t i = 0;
    for (const QFileInfo& file : files) {
        setProgress(i, files.length());
        QString relative = mc.relativeFilePath(file.absoluteFilePath());
        if (!resolvedFiles.contains(relative) && !JlCompress::compressFile(&zip, file.absoluteFilePath(), "overrides/" + relative))
            qWarning() << "Could not compress" << file;
        i++;
    }

    zip.close();

    if (zip.getZipError() != 0) {
        QFile::remove(output);
        emitFailed(tr("A zip error occured"));
        return;
    }

    emitSucceeded();
}

QByteArray ModrinthPackExportTask::generateIndex(const QMap<QString, ResolvedFile>& resolvedFiles)
{
    QJsonObject obj;
    obj["formatVersion"] = 1;
    obj["game"] = "minecraft";
    obj["name"] = name;
    obj["versionId"] = version;
    obj["summary"] = summary;

    MinecraftInstance* mc = dynamic_cast<MinecraftInstance*>(instance.get());
    if (mc) {
        auto profile = mc->getPackProfile();
        // collect all supported components
        auto minecraft = profile->getComponent("net.minecraft");
        auto quilt = profile->getComponent("org.quiltmc.quilt-loader");
        auto fabric = profile->getComponent("net.fabricmc.fabric-loader");
        auto forge = profile->getComponent("net.minecraftforge");

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
        file["path"] = iterator.key();
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