// SPDX-License-Identifier: GPL-3.0-only
/*
 *  MaterialMC - Minecraft Launcher
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

#include <QHash>
#include <QObject>
#include <QSet>
#include <QVector>

#include "modplatform/ModIndex.h"
#include "tasks/Task.h"

namespace api {

class ApiRouter;
class TaskTracker;

/**
 * `mods.*`: search Modrinth / CurseForge through the existing ResourceAPI implementations and install
 * with ResourceDownloadTask. Search results are cached here because installing needs the full
 * IndexedPack / IndexedVersion objects; the UI only ever refers to them by provider + id.
 */
class ModApi : public QObject {
    Q_OBJECT
   public:
    ModApi(ApiRouter* router, TaskTracker* tasks, QObject* parent = nullptr);

   private:
    void registerMethods();
    void runRequest(const Task::Ptr& task);
    static QString cacheKey(ModPlatform::ResourceProvider provider, const QString& projectId);

    ApiRouter* m_router;
    TaskTracker* m_tasks;
    QHash<QString, ModPlatform::IndexedPack::Ptr> m_packs;
    QHash<QString, QVector<ModPlatform::IndexedVersion>> m_versions;
    QSet<Task::Ptr> m_requests;
};

}  // namespace api
