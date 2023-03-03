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

    QFileInfoList files;
    if (!MMCZip::collectFileListRecursively(instance->gameRoot(), nullptr, &files, filter)) {
        emitFailed(tr("Could not collect list of files"));
        return;
    }

    QDir mc(instance->gameRoot());

    ModrinthAPI api;

    static const QStringList prefixes({ "mods", "coremods", "resourcepacks", "texturepacks", "shaderpacks" });
    // hash -> file
    QMap<QString, QString> hashes;

    for (QFileInfo file : files) {
        QString relative = mc.relativeFilePath(file.absoluteFilePath());
        // require sensible file types
        if (!(relative.endsWith(".zip") || relative.endsWith(".jar") || relative.endsWith(".litemod")))
            continue;

        if (!std::any_of(prefixes.begin(), prefixes.end(),
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

        hashes[hash.result().toHex()] = relative;
    }

    QByteArray* response = new QByteArray;
    Task::Ptr versionsTask = api.currentVersions(hashes.keys(), "sha512", response);
    connect(versionsTask.get(), &NetJob::succeeded, this, [this, mc, files, hashes, response, versionsTask]() {
        // file -> url
        QMap<QString, ResolvedFile> resolved;

        try {
            QJsonDocument doc = Json::requireDocument(*response);
            for (auto iter = hashes.keyBegin(); iter != hashes.keyEnd(); iter++) {
                QJsonObject obj = doc[*iter].toObject();
                if (obj.isEmpty())
                    continue;

                QJsonArray files = obj["files"].toArray();
                if (auto fileIter = std::find_if(files.begin(), files.end(),
                                                 [&iter](const QJsonValue& file) { return file["hashes"]["sha512"] == *iter; });
                    fileIter != files.end()) {
                    // map the file to the url
                    resolved[hashes[*iter]] = ResolvedFile{ fileIter->toObject()["hashes"].toObject()["sha1"].toString(), *iter,
                                                            fileIter->toObject()["url"].toString(), fileIter->toObject()["size"].toInt() };
                }
            }
        } catch (Json::JsonException& e) {
            qWarning() << "Failed to parse versions response" << e.what();
        }

        delete response;

        setStatus("Adding files...");

        QuaZip zip(output);
        if (!zip.open(QuaZip::mdCreate)) {
            QFile::remove(output);
            emitFailed(tr("Could not create file"));
            return;
        }

        {
            QuaZipFile indexFile(&zip);
            if (!indexFile.open(QIODevice::WriteOnly, QuaZipNewInfo("modrinth.index.json"))) {
                QFile::remove(output);
                emitFailed(tr("Could not create index"));
                return;
            }
            indexFile.write(generateIndex(resolved));
        }

        {
            size_t i = 0;
            for (const QFileInfo& file : files) {
                setProgress(i, files.length());
                QString relative = mc.relativeFilePath(file.absoluteFilePath());
                if (!resolved.contains(relative) && !JlCompress::compressFile(&zip, file.absoluteFilePath(), "overrides/" + relative))
                    qWarning() << "Could not compress" << file;
                i++;
            }
        }

        zip.close();

        if (zip.getZipError() != 0) {
            QFile::remove(output);
            emitFailed(tr("A zip error occured"));
            return;
        }

        emitSucceeded();
    });
    connect(versionsTask.get(), &NetJob::failed, this, [this](const QString& reason) { emitFailed(reason); });
    versionsTask->start();
}

QByteArray ModrinthPackExportTask::generateIndex(const QMap<QString, ResolvedFile>& urls)
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
    QMapIterator<QString, ResolvedFile> iterator(urls);
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