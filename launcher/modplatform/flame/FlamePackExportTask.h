// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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

#include "MMCZip.h"
#include "minecraft/MinecraftInstance.h"
#include "modplatform/flame/FlameAPI.h"
#include "tasks/Task.h"

struct FlamePackExportOptions {
    QString name;
    QString version;
    QString author;
    bool optionalFiles;
    MinecraftInstance* instance;
    QString output;
    MMCZip::FilterFileFunction filter;
    int recommendedRAM;
};

class FlamePackExportTask : public Task {
    Q_OBJECT
   public:
    FlamePackExportTask(FlamePackExportOptions&& options);

   protected:
    void executeTask() override;
    bool abort() override;

   private:
    static const QString TEMPLATE;
    static const QStringList FILE_EXTENSIONS;

    // inputs

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

    FlamePackExportOptions m_options;
    QDir m_gameRoot;

    FlameAPI api;

    QFileInfoList files;
    QMap<QString, HashInfo> pendingHashes{};
    QMap<QString, ResolvedFile> resolvedFiles{};
    Task::Ptr task;

    void collectFiles();
    void collectHashes();
    void makeApiRequest();
    void getProjectsInfo();
    void buildZip();

    QByteArray generateIndex();
    QByteArray generateHTML();
};
