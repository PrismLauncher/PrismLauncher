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

#pragma once

#include <QFuture>
#include <QFutureWatcher>
#include "BaseInstance.h"
#include "MMCZip.h"
#include "minecraft/MinecraftInstance.h"
#include "modplatform/flame/FlameAPI.h"
#include "tasks/Task.h"

class FlamePackExportTask : public Task {
   public:
    FlamePackExportTask(const QString& name,
                        const QString& version,
                        const QString& author,
                        const QVariant& projectID,
                        InstancePtr instance,
                        const QString& output,
                        MMCZip::FilterFunction filter);

   protected:
    void executeTask() override;
    bool abort() override;

   private:
    static const QString TEMPLATE;

    // inputs
    const QString name, version, author;
    const QVariant projectID;
    const InstancePtr instance;
    MinecraftInstance* mcInstance;
    const QDir gameRoot;
    const QString output;
    const MMCZip::FilterFunction filter;

    typedef std::optional<QString> BuildZipResult;
    struct ResolvedFile {
        int addonId;
        int version;
        bool enabled;
        bool isMod;

        QString name;
        QString slug;
        QString authors;
    };
    struct HashInfo {
        QString name;
        QString path;
        bool enabled;
        bool isMod;
    };

    FlameAPI api;

    QFileInfoList files;
    QMap<QString, HashInfo> pendingHashes{};
    QMap<QString, ResolvedFile> resolvedFiles{};
    Task::Ptr task;
    QFuture<BuildZipResult> buildZipFuture;
    QFutureWatcher<BuildZipResult> buildZipWatcher;

    void collectFiles();
    void collectHashes();
    void makeApiRequest();
    void getProjectsInfo();
    void buildZip();
    void finish();

    QByteArray generateIndex();
};
