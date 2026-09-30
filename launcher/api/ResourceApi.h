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

#include <QObject>
#include <QSet>
#include <QString>

#include <functional>

class MinecraftInstance;
class ResourceFolderModel;
class WorldList;

namespace api {

class ApiRouter;
class TaskTracker;

/**
 * Instance content: `resources.*` (mods, resource/shader/texture packs), `worlds.*`, `screenshots.*`, `logs.*`.
 * Works on the existing ResourceFolderModel / WorldList objects of each MinecraftInstance.
 */
class ResourceApi : public QObject {
    Q_OBJECT
   public:
    ResourceApi(ApiRouter* router, TaskTracker* tasks, QObject* parent = nullptr);

    /** Maps a `ResourceKind` name to the instance's folder model; throws INVALID_PARAMS for unknown kinds. */
    static ResourceFolderModel* modelFor(MinecraftInstance* instance, const QString& kind);

    /** Absolute path of a screenshot of `instance`, or empty if `name` is not a screenshot file there. */
    static QString screenshotPath(MinecraftInstance* instance, const QString& name);
    /** Absolute path of a world's icon.png, or empty. */
    static QString worldIconPath(MinecraftInstance* instance, const QString& worldId);

   private:
    void registerMethods();
    /** Loads `model` the first time it is used and forwards its updates as `resources.changed`. */
    void ensureLoaded(MinecraftInstance* instance, const QString& kind, ResourceFolderModel* model, std::function<void()> then);
    void ensureWorldsLoaded(MinecraftInstance* instance, WorldList* worlds);

    ApiRouter* m_router;
    TaskTracker* m_tasks;
    QSet<ResourceFolderModel*> m_loadedModels;
    QSet<WorldList*> m_loadedWorlds;
};

}  // namespace api
