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

#include "VersionApi.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>

#include "Application.h"
#include "BaseVersionList.h"
#include "java/JavaInstall.h"
#include "java/JavaInstallList.h"
#include "meta/Index.h"
#include "meta/Version.h"
#include "meta/VersionList.h"

#include "ApiRouter.h"
#include "ApiUtils.h"
#include "TaskTracker.h"

namespace api {

namespace {

const char* loaderUid(const QString& kind)
{
    if (kind == QLatin1String("fabric")) {
        return "net.fabricmc.fabric-loader";
    }
    if (kind == QLatin1String("quilt")) {
        return "org.quiltmc.quilt-loader";
    }
    if (kind == QLatin1String("forge")) {
        return "net.minecraftforge";
    }
    if (kind == QLatin1String("neoforge")) {
        return "net.neoforged";
    }
    if (kind == QLatin1String("liteloader")) {
        return "com.mumfrey.liteloader";
    }
    throw ApiError::invalidParams(QObject::tr("Unknown mod loader '%1'").arg(kind));
}

/**
 * Makes sure `list` is loaded (from the cache or the network), then calls `then`.
 * Loading runs as a tracked task so it shows up in the downloads view.
 */
void withLoadedList(BaseVersionList* list,
                    bool forceReload,
                    TaskTracker* tasks,
                    QObject* context,
                    const QString& title,
                    const ApiReply& reply,
                    std::function<QJsonValue()> then)
{
    if (list->isLoaded() && !forceReload) {
        reply.guard([&] { reply.resolve(then()); });
        return;
    }
    auto task = list->getLoadTask(forceReload);
    if (!task) {
        reply.guard([&] { reply.resolve(then()); });
        return;
    }
    QObject::connect(
        task.get(), &Task::finished, context,
        [task, reply, then] {
            if (!task->wasSuccessful()) {
                reply.reject(ApiError("NETWORK_ERROR", QObject::tr("Could not load the list: %1").arg(task->failReason())));
                return;
            }
            reply.guard([&] { reply.resolve(then()); });
        },
        Qt::SingleShotConnection);
    if (!task->isRunning()) {
        tasks->start(task, "versions.load", title);
    }
}

QJsonObject serializeMetaVersion(const Meta::Version::Ptr& v)
{
    return {
        { "version", v->version() },
        { "type", v->type() },
        { "releaseTime", timestamp(v->time()) },
        { "recommended", v->isRecommended() },
    };
}

}  // namespace

void registerVersionApi(ApiRouter* router, TaskTracker* tasks, QObject* context)
{
    router->add("versions.minecraft", [tasks, context](const QJsonObject& p, const ApiReply& reply) {
        auto list = APPLICATION->metadataIndex()->get("net.minecraft");
        withLoadedList(list.get(), params::optionalBool(p, "forceReload", false), tasks, context, QObject::tr("Loading Minecraft versions"),
                       reply, [list] {
                           QJsonArray out;
                           for (const auto& v : list->versions()) {
                               out.append(QJsonObject{
                                   { "id", v->version() },
                                   { "type", v->type() },
                                   { "releaseTime", timestamp(v->time()) },
                                   { "recommended", v->isRecommended() },
                               });
                           }
                           return out;
                       });
    });

    router->add("versions.loaders", [tasks, context](const QJsonObject& p, const ApiReply& reply) {
        const auto kind = params::requireNonEmpty(p, "loader", 32);
        const QString uid = QString::fromLatin1(loaderUid(kind));
        const auto mcVersion = params::requireNonEmpty(p, "minecraftVersion", 128);
        // Fabric and Quilt loaders are not tied to a Minecraft version (the intermediary mappings are).
        const bool versionIndependent = kind == QLatin1String("fabric") || kind == QLatin1String("quilt");
        auto list = APPLICATION->metadataIndex()->get(uid);
        withLoadedList(list.get(), params::optionalBool(p, "forceReload", false), tasks, context,
                       QObject::tr("Loading %1 versions").arg(list->humanReadable()), reply, [list, mcVersion, versionIndependent] {
                           QJsonArray out;
                           for (const auto& v : list->versions()) {
                               const auto& reqs = v->requiredSet();
                               const auto mc = std::find_if(reqs.begin(), reqs.end(), [](const Meta::Require& r) { return r.uid == "net.minecraft"; });
                               const QString parent = mc != reqs.end() ? mc->equalsVersion : QString();
                               if (versionIndependent ? parent.isEmpty() : parent == mcVersion) {
                                   out.append(serializeMetaVersion(v));
                               }
                           }
                           return out;
                       });
    });

    router->add("java.list", [tasks, context](const QJsonObject& p, const ApiReply& reply) {
        auto* list = APPLICATION->javalist();
        withLoadedList(list, params::optionalBool(p, "forceReload", false), tasks, context, QObject::tr("Detecting Java installations"), reply,
                       [list] {
                           const auto managedRoot = QDir(APPLICATION->javaPath()).absolutePath();
                           QJsonArray out;
                           for (int i = 0; i < list->count(); i++) {
                               const auto java = std::dynamic_pointer_cast<JavaInstall>(list->at(i));
                               if (!java) {
                                   continue;
                               }
                               out.append(QJsonObject{
                                   { "path", java->path },
                                   { "version", java->id.toString() },
                                   { "architecture", java->arch },
                                   { "recommended", list->data(list->index(i), BaseVersionList::RecommendedRole).toBool() },
                                   { "isManaged", QFileInfo(java->path).isAbsolute() && QDir(java->path).absolutePath().startsWith(managedRoot) },
                               });
                           }
                           return out;
                       });
    });
}

}  // namespace api
