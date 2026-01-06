// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024
 *  Copyright (c) 2024 abhicommands <114682464+abhicommands@users.noreply.github.com>
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

#include <QDir>
#include <QMap>
#include <QObject>
#include <atomic>
#include <memory>

#include "minecraft/mod/Resource.h"
#include "tasks/Task.h"

class ModFolderLoadTask : public Task {
    Q_OBJECT
   public:
    struct Result {
        QMap<QString, Resource::Ptr> resources;
    };
    using ResultPtr = std::shared_ptr<Result>;
    ResultPtr result() const { return m_result; }

   public:
    ModFolderLoadTask(const QDir& rootDir, bool isIndexed, bool cleanOrphan, std::function<Resource*(const QFileInfo&)> createFunction);

    bool canAbort() const override { return true; }
    bool abort() override
    {
        m_aborted.store(true);
        return true;
    }

    void executeTask() override;

   private:
    void getFromMetadata(const QDir& dir);
    void getFromFiles(const QDir& dir, int depth);
    QString internalIdForFile(const QFileInfo& info) const;

   private:
    QDir m_rootDir;
    bool m_isIndexed;
    bool m_cleanOrphan;
    std::function<Resource*(const QFileInfo&)> m_createFunc;
    ResultPtr m_result;

    std::atomic<bool> m_aborted = false;

    QThread* m_threadToSpawnInto;
};
