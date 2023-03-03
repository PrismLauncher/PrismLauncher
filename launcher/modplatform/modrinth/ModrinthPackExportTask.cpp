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
#include <qtconcurrentrun.h>
#include <QFileInfoList>
#include <QMessageBox>
#include "MMCZip.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

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
    QtConcurrent::run(QThreadPool::globalInstance(), [this] {
        QFileInfoList files;
        if (!MMCZip::collectFileListRecursively(instance->gameRoot(), nullptr, &files, filter)) {
            emitFailed(tr("Could not collect list of files"));
            return;
        }

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
            indexFile.write(generateIndex());
        }

        // should exist
        QDir dotMinecraft(instance->gameRoot());

        {
            size_t i = 0;
            for (const QFileInfo& file : files) {
                setProgress(i, files.length());
                if (!JlCompress::compressFile(&zip, file.absoluteFilePath(),
                                              "overrides/" + dotMinecraft.relativeFilePath(file.absoluteFilePath()))) {
                    emitFailed(tr("Could not compress %1").arg(file.absoluteFilePath()));
                    return;
                }
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
}

QByteArray ModrinthPackExportTask::generateIndex()
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
        auto minecraft = profile->getComponent("net.minecraft");

        QJsonObject dependencies;
        dependencies["minecraft"] = minecraft->m_version;
        obj["dependencies"] = dependencies;
    }

    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}