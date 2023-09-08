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

#pragma once

#include <QFuture>
#include <QFutureWatcher>
#include "BaseInstance.h"
#include "MMCZip.h"
#include "minecraft/MinecraftInstance.h"
#include "modplatform/modrinth/ModrinthAPI.h"
#include "tasks/Task.h"

class ModrinthPackExportTask : public Task {
   public:
    ModrinthPackExportTask(const QString& name,
                           const QString& version,
                           const QString& summary,
                           bool optionalFiles,
                           InstancePtr instance,
                           const QString& output,
                           MMCZip::FilterFunction filter);

   protected:
    void executeTask() override;
    bool abort() override;

   private:
    struct ResolvedFile {
        QString sha1, sha512, url;
        qint64 size;
    };

    static const QStringList PREFIXES;
    static const QStringList FILE_EXTENSIONS;

    // inputs
    const QString name, version, summary;
    const bool optionalFiles;
    const InstancePtr instance;
    MinecraftInstance* mcInstance;
    const QDir gameRoot;
    const QString output;
    const MMCZip::FilterFunction filter;

    ModrinthAPI api;
    QFileInfoList files;
    QMap<QString, QString> pendingHashes;
    QMap<QString, ResolvedFile> resolvedFiles;
    Task::Ptr task;

    void collectFiles();
    void collectHashes();
    void makeApiRequest();
    void parseApiResponse(const std::shared_ptr<QByteArray> response);
    void buildZip();

    QByteArray generateIndex();
};
