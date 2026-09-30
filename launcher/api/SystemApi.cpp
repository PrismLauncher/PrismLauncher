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

#include "SystemApi.h"

#include <QClipboard>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonArray>
#include <QSaveFile>
#include <QStandardPaths>
#include <QSysInfo>

#include "Application.h"
#include "BuildConfig.h"
#include "DesktopServices.h"
#include "FileSystem.h"
#include "InstanceList.h"
#include "icons/IconList.h"
#include "minecraft/MinecraftInstance.h"
#include "settings/SettingsObject.h"

#include "ApiRouter.h"
#include "ApiUtils.h"
#include "TaskTracker.h"

namespace api {

QString resolveFolderTarget(const QString& target, MinecraftInstance* instance)
{
    if (target == QLatin1String("data")) {
        return APPLICATION->dataRoot();
    }
    if (target == QLatin1String("instances")) {
        return APPLICATION->instances()->primaryDir();
    }
    if (target == QLatin1String("icons")) {
        return APPLICATION->icons()->getDirectory();
    }
    if (target == QLatin1String("logs")) {
        return FS::PathCombine(APPLICATION->dataRoot(), "logs");
    }
    if (target == QLatin1String("java")) {
        return APPLICATION->javaPath();
    }
    if (!target.startsWith(QLatin1String("instance"))) {
        throw ApiError::invalidParams(QObject::tr("Unknown folder '%1'").arg(target));
    }
    if (!instance) {
        throw ApiError::invalidParams(QObject::tr("Folder '%1' needs an instanceId").arg(target));
    }
    if (target == QLatin1String("instance")) {
        return instance->instanceRoot();
    }
    if (target == QLatin1String("instance.game")) {
        return instance->gameRoot();
    }
    if (target == QLatin1String("instance.mods")) {
        return instance->modsRoot();
    }
    if (target == QLatin1String("instance.resourcepacks")) {
        return instance->resourcePacksDir();
    }
    if (target == QLatin1String("instance.shaderpacks")) {
        return instance->shaderPacksDir();
    }
    if (target == QLatin1String("instance.texturepacks")) {
        return instance->texturePacksDir();
    }
    if (target == QLatin1String("instance.saves")) {
        return instance->worldDir();
    }
    if (target == QLatin1String("instance.screenshots")) {
        return FS::PathCombine(instance->gameRoot(), "screenshots");
    }
    if (target == QLatin1String("instance.logs")) {
        return FS::PathCombine(instance->gameRoot(), "logs");
    }
    throw ApiError::invalidParams(QObject::tr("Unknown folder '%1'").arg(target));
}

void registerSystemApi(ApiRouter* router, TaskTracker* tasks, const HostInfo& host)
{
    router->addSync("system.info", [host](const QJsonObject&) {
        const auto caps = APPLICATION->capabilities();
        return QJsonObject{
            { "name", BuildConfig.LAUNCHER_NAME },
            { "displayName", BuildConfig.LAUNCHER_DISPLAYNAME },
            { "version", BuildConfig.printableVersionString() },
            { "gitCommit", BuildConfig.GIT_COMMIT },
            { "buildPlatform", BuildConfig.BUILD_PLATFORM },
            { "os", QSysInfo::prettyProductName() },
            { "qtVersion", QString::fromLatin1(qVersion()) },
            { "webViewEngine", host.webViewEngine },
            { "isPortable", APPLICATION->isPortable() },
            { "dataPath", APPLICATION->dataRoot() },
            { "devMode", host.devMode },
            { "capabilities",
              QJsonObject{
                  { "msa", caps.testFlag(Application::SupportsMSA) },
                  { "curseforge", caps.testFlag(Application::SupportsFlame) },
                  { "gameMode", caps.testFlag(Application::SupportsGameMode) },
                  { "mangoHud", caps.testFlag(Application::SupportsMangoHud) },
              } },
            { "urls",
              QJsonObject{
                  { "bugTracker", BuildConfig.BUG_TRACKER_URL },
                  { "wiki", BuildConfig.WIKI_URL },
                  { "discord", BuildConfig.DISCORD_URL },
                  { "matrix", BuildConfig.MATRIX_URL },
                  { "subreddit", BuildConfig.SUBREDDIT_URL },
                  { "translations", BuildConfig.TRANSLATIONS_URL },
              } },
        };
    });

    router->addSync("system.methods", [router](const QJsonObject&) { return QJsonArray::fromStringList(router->methods()); });

    router->addSync("system.openFolder", [](const QJsonObject& p) {
        const auto target = params::requireNonEmpty(p, "target", 64);
        MinecraftInstance* instance = nullptr;
        if (!params::isMissing(p.value("instanceId"))) {
            instance = requireInstance(p, "instanceId");
        }
        const auto path = resolveFolderTarget(target, instance);
        if (path.isEmpty() || !DesktopServices::openPath(path, true)) {
            throw ApiError::io(QObject::tr("Could not open %1").arg(path));
        }
        return ok();
    });

    router->addSync("system.openUrl", [](const QJsonObject& p) {
        const QUrl url(params::requireNonEmpty(p, "url", 4096), QUrl::StrictMode);
        // Only web links; never file://, custom schemes or anything that could start a local program.
        if (!url.isValid() || (url.scheme() != QLatin1String("https") && url.scheme() != QLatin1String("http")) || url.host().isEmpty()) {
            throw ApiError::permissionDenied(QObject::tr("Only http(s) links can be opened"));
        }
        if (!DesktopServices::openUrl(url)) {
            throw ApiError::io(QObject::tr("Could not open the browser"));
        }
        return ok();
    });

    router->addSync("system.copyText", [](const QJsonObject& p) {
        QGuiApplication::clipboard()->setText(params::requireString(p, "text", 32 * 1024 * 1024));
        return ok();
    });

    router->addSync("system.saveText", [](const QJsonObject& p) {
        // The user picks the destination in a native dialog; the page only suggests a file name.
        auto suggested = QFileInfo(params::requireNonEmpty(p, "suggestedName", 255)).fileName();
        const auto content = params::requireString(p, "content", 64 * 1024 * 1024);
        const auto dir = APPLICATION->settings()->get("DownloadsDir").toString();
        const auto path = QFileDialog::getSaveFileName(nullptr, QObject::tr("Save file"), FS::PathCombine(dir, suggested));
        if (path.isEmpty()) {
            return QJsonValue(QJsonObject{ { "saved", false } });
        }
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(content.toUtf8()) < 0 || !file.commit()) {
            throw ApiError::io(QObject::tr("Could not write %1: %2").arg(path, file.errorString()));
        }
        return QJsonValue(QJsonObject{ { "saved", true } });
    });

    router->addSync("system.icons", [](const QJsonObject&) {
        QJsonArray out;
        auto* icons = APPLICATION->icons();
        for (int i = 0; i < icons->rowCount(); i++) {
            const auto index = icons->index(i);
            const auto key = icons->data(index, Qt::UserRole).toString();
            out.append(QJsonObject{ { "key", key },
                                    { "name", icons->data(index, Qt::DisplayRole).toString() },
                                    { "url", hostResourceUrl({ "_icon", key }) } });
        }
        return out;
    });

    router->addSync("tasks.list", [tasks](const QJsonObject&) { return tasks->list(); });
    router->addSync("tasks.cancel", [tasks](const QJsonObject& p) {
        if (!tasks->cancel(params::requireNonEmpty(p, "taskId", 64))) {
            throw ApiError("UNSUPPORTED", QObject::tr("This task cannot be cancelled"));
        }
        return ok();
    });
    router->addSync("tasks.clearFinished", [tasks](const QJsonObject&) {
        tasks->clearFinished();
        return ok();
    });
}

}  // namespace api
