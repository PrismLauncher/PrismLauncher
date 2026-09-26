// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 TheKodeToad <TheKodeToad@proton.me>
 *  Copyright (C) 2025 Yihe Li <winmikedows@hotmail.com>
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

#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

#include "InstanceConfig.h"
#include "INIFile.h"
#include "Json.h"

using namespace Qt::Literals;

namespace {
InstanceConfig loadConfig(const INIFile& file)
{
    InstanceConfig conf{};

    // NOTE: all new keys should be added with UpperCamelCase for consistency
    // The lowerCamelCase keys are retained purely so that older launcher versions can understand the file format

    conf.name = file.convert<QString>("name", "Unnamed Instance");
    conf.iconKey = file.convert<QString>("iconKey", "default");
    conf.notes = file.convert<QString>("notes");

    conf.lastLaunchTime = file.convert<qint64>("lastLaunchTime");
    conf.totalTimePlayed = std::max<int64_t>(file.convert<qint64>("totalTimePlayed"), 0);
    conf.lastTimePlayed = file.convert<qint64>("lastTimePlayed");

    conf.linkedInstances = Json::toStringList(file.convert<QString>("linkedInstances", "[]"));

    const auto shortcutsJson = file.convert<QByteArray>("shortcuts", "[]");
    const auto shortcutsDoc = QJsonDocument::fromJson(shortcutsJson);
    if (shortcutsDoc.isArray()) {
        for (const auto shortcut : shortcutsDoc.array()) {
            if (!shortcut.isObject()) {
                qWarning() << u"Non-object value in instance shortcuts";
                continue;
            }

            const auto shortcutObj = shortcut.toObject();
            const auto shortcutName = shortcutObj["name"];
            const auto filePath = shortcutObj["filePath"];
            const auto target = shortcutObj["target"];

            if (!shortcutName.isString() || !filePath.isString() || !target.isDouble()) {
                qWarning() << u"Expected shape { name: string, filePath: string, target: number } for instance shortcut";
                continue;
            }

            const auto targetVal = target.toInt();
            if (targetVal < 0 || targetVal >= static_cast<int>(InstanceConfig::ShortcutTarget::Count)) {
                qWarning() << u"Found invalid instance shortcut target type";
                continue;
            }

            conf.shortcuts.append(InstanceConfig::Shortcut{
                .name = shortcutName.toString(),
                .filePath = filePath.toString(),
                .target = static_cast<InstanceConfig::ShortcutTarget>(target.toInt()),
            });
        }
    } else {
        qWarning() << u"Expected JSON array under instance shortcuts";
    }

    conf.uuid = file.convert<QString>("uuid");

    bool overrideGameTime = file.convert("OverrideGameTime", false);
    if (overrideGameTime) {
        conf.gameTime = GlobalConfig::GameTimeOverrides{
            .show = file.convert("ShowGameTime", true),
            .record = file.convert("RecordGameTime", true),
        };
    }

    conf.countGameTime = file.convert("CountGameTime", true);

    bool overrideCommands = file.convert("OverrideCommands", false);
    if (overrideCommands) {
        conf.commands = GlobalConfig::CommandOverrides{
            .preLoad = file.convert<QString>("PreLoadCommand"),
            .preLaunch = file.convert<QString>("PreLaunchCommand"),
            .wrapper = file.convert<QString>("WrapperCommand"),
            .postExit = file.convert<QString>("PostExitCommand"),
        };
    }

    bool overrideConsole = file.convert("OverrideConsole", false);
    if (overrideConsole) {
        conf.console = GlobalConfig::ConsoleOverrides{
            .show = file.convert("ShowConsole", false),
            .autoClose = file.convert("AutoCloseConsole", false),
            .showOnError = file.convert("ShowConsoleOnError", true),
        };
    }

    bool isManagedPack = file.convert("ManagedPack", false);
    if (isManagedPack) {
        conf.managedPack = InstanceConfig::ManagedPack{
            .type = file.convert<QString>("ManagedPackType"),
            .id = file.convert<QString>("ManagedPackID"),
            .name = file.convert<QString>("ManagedPackName"),
            .versionId = file.convert<QString>("ManagedPackVersionID"),
            .versionName = file.convert<QString>("ManagedPackVersionName"),
            .url = file.convert<QString>("ManagedPackURL"),
        };
    }

    conf.profiler = file.convert<QString>("Profiler");

    const auto overrideJavaInstallation = file.convert("OverrideJavaLocation", false);
    if (overrideJavaInstallation) {
        conf.javaInstallation = GlobalConfig::JavaInstallationOverrides{
            .path = file.convert<QString>("JavaPath"),
            .signature = file.convert<QString>("JavaSignature"),
            .architecture = file.convert<QString>("JavaArchitecture"),
            .realArchitecture = file.convert<QString>("JavaRealArchitecture"),
            .version = file.convert<QString>("JavaVersion"),
            .vendor = file.convert<QString>("JavaVendor"),
            .ignoreCompatibility = file.convert("IgnoreJavaCompatibility", false),
        };
    }

    conf.automaticJava = file.convert("AutomaticJava", false);

    // NOTE: it was previously two group boxes, then merged into one
    const auto overrideWindow = file.convert("OverrideWindow", false) || file.convert("OverrideMiscellaneous", false);
    if (overrideWindow) {
        conf.gameWindow = GlobalConfig::GameWindowOverrides{
            .maximized = file.convert("LaunchMaximized", true),
            .width = file.convert("MinecraftWinWidth", 854),
            .height = file.convert("MinecraftWinHeight", 480),
            .hideLauncherOnOpen = file.convert("CloseAfterLaunch", false),
            .quitLauncherOnClose = file.convert("QuitAfterGameStop", false),
        };
    }

    const auto overrideMemory = file.convert("OverrideMemory", false);
    if (overrideMemory) {
        const auto memoryDefaults = GlobalConfig::MemoryOverrides::defaults();
        conf.memory = GlobalConfig::MemoryOverrides{
            .minAlloc = file.convert("MinMemAlloc", memoryDefaults.minAlloc),
            .maxAlloc = file.convert("MaxMemAlloc", memoryDefaults.maxAlloc),
            .permGen = file.convert("PermGen", memoryDefaults.permGen),
            .lowMemWarning = file.convert("LowMemWarning", memoryDefaults.lowMemWarning),
        };
    }

    const auto overrideJvmArgs = file.convert("OverrideJavaArgs", false);
    if (overrideJvmArgs) {
        conf.jvmArgs = file.convert<QString>("JvmArgs");
    }

    const auto overrideNativeLibraries = file.convert("OverrideNativeWorkarounds", false);
    if (overrideNativeLibraries) {
        conf.nativeLibraries = GlobalConfig::NativeLibraryOverrides{
            .glfw = file.convert("UseNativeGLFW", false),
            .customGLFWPath = file.convert<QString>("CustomGLFWPath"),
            .openAL = file.convert("UseNativeOpenAL", false),
            .customOpenALPath = file.convert<QString>("CustomOpenALPath"),
            .sdl = file.convert("UseNativeSDL", false),
            .customSDLPath = file.convert<QString>("CustomSDLPath"),
        };
    }

    const auto overridePerformance = file.convert("OverridePerformance", false);
    if (overridePerformance) {
        conf.performance = GlobalConfig::PerformanceOverrides{
            .enableFeralGamemode = file.convert("EnableFeralGamemode", false),
            .enableMangoHud = file.convert("EnableMangoHud", false),
            .useDiscreteGpu = file.convert("UseDiscreteGpu", false),
            .useZink = file.convert("UseZink", false),
        };
    }

    const auto overrideLegacySettings = file.convert("OverrideLegacySettings", false);
    if (overrideLegacySettings) {
        conf.legacy = GlobalConfig::LegacyOverrides{
            .onlineFixes = file.convert("OnlineFixes", false),
        };
    }

    const auto overrideEnv = file.convert("OverrideEnv", false);
    if (overrideEnv) {
        conf.env = Json::toMap(file.convert<QString>("Env", "{}"));
    }

    const auto overrideDefaultAccount = file.convert("UseAccountForInstance", false);
    if (overrideDefaultAccount) {
        conf.defaultAccount = file.convert<QString>("InstanceAccountId");
    }

    const auto autoJoinEnabled = file.convert("JoinServerOnLaunch", false);
    if (autoJoinEnabled) {
        if (const auto serverAddr = file.convert<QString>("JoinServerOnLaunchAddress"); !serverAddr.isEmpty()) {
            conf.joinOnLaunch = InstanceConfig::ServerJoinTarget{ serverAddr };
        } else if (const auto world = file.convert<QString>("JoinWorldOnLaunch"); !world.isEmpty()) {
            conf.joinOnLaunch = InstanceConfig::WorldJoinTarget{ serverAddr };
        }
    }

    conf.exportName = file.convert<QString>("ExportName");
    conf.exportVersion = file.convert<QString>("ExportVersion");
    conf.exportSummary = file.convert<QString>("ExportSummary");
    conf.exportAuthor = file.convert<QString>("ExportAuthor");
    conf.exportOptionalFiles = file.convert("ExportOptionalFiles", true);
    conf.exportRecommendedRam = file.convert<int>("ExportRecommendedRAM");

    const auto globalDataPacksEnabled = file.convert("GlobalDataPacksEnabled", false);
    if (globalDataPacksEnabled) {
        conf.globalDataPacksPath = file.convert<QString>("GlobalDataPacksPath");
    }

    const auto overrideModDownloadLoaders = file.convert("OverrideModDownloadLoaders", false);
    if (overrideModDownloadLoaders) {
        conf.modDownloadLoaders = Json::toStringList(file.convert<QString>("ModDownloadLoaders", "[]"));
    }

    const auto useLatestMinecraftVersion = file.convert("UseLatestMinecraftVersion", false);
    if (useLatestMinecraftVersion) {
        conf.useLatestMinecraftVersionType = file.convert<QString>("UseLatestMinecraftVersionType", "release");
    }

    for (auto iter = file.begin(); iter != file.end(); ++iter) {
        QString key = iter.key();
        auto removePrefix = [](QString& key, QStringView prefix) {
            if (!key.startsWith(prefix)) {
                return false;
            }

            key = key.mid(prefix.length());
            return true;
        };

        if (removePrefix(key, u"UIColumnVisibility/"_s)) {
            const auto doc = QJsonDocument::fromJson(iter.value().toByteArray());
            if (!doc.isObject()) {
                qWarning() << u"Expected JSON object for instance column visibility at" << iter.key();
                continue;
            }

            const auto obj = doc.object();
            QHash<QString, bool> map;

            for (auto objIter = obj.begin(); objIter != obj.end(); ++objIter) {
                map[objIter.key()] = objIter.value().toBool();
            }

            conf.uiColumnVisibility[key] = std::move(map);
        } else if (removePrefix(key, u"UIColumnSizes/")) {
            const auto doc = QJsonDocument::fromJson(iter.value().toByteArray());
            if (!doc.isObject()) {
                qWarning() << u"Expected JSON object for instance column sizes at" << iter.key();
                continue;
            }

            const auto obj = doc.object();
            QHash<QString, int> map;

            for (auto objIter = obj.begin(); objIter != obj.end(); ++objIter) {
                map[objIter.key()] = objIter.value().toInt();
            }

            conf.uiColumnSizes[key] = std::move(map);
        }
    }

    return conf;
}
}  // namespace

Result<InstanceConfig> InstanceConfig::load(const QString& path)
{
    qDebug() << u"Loading instance config from" << path;

    INIFile file;
    TRY(file.loadFile(path));

    const auto& type = file.value("InstanceType").toString();
    if (!type.isEmpty() && type != "OneSix") {
        return std::unexpected("Unrecognized instance type: " + type);
    }

    return loadConfig(file);
}

InstanceConfig InstanceConfig::loadDefaults()
{
    return loadConfig(INIFile());
}

Result<> InstanceConfig::save(const QString& path) const
{
    qDebug() << u"Saving instance config to" << path;

    INIFile file;
    file["InstanceType"] = "OneSix";

    file["name"] = name;
    file["iconKey"] = iconKey;
    file["notes"] = notes;

    file["lastLaunchTime"] = qint64{ lastLaunchTime };
    file["totalTimePlayed"] = qint64{ totalTimePlayed };
    file["lastTimePlayed"] = qint64{ lastTimePlayed };

    file["linkedInstances"] = Json::fromStringList(linkedInstances);

    QJsonArray shortcutsArray;
    for (const auto& shortcut : shortcuts) {
        shortcutsArray.append(QJsonObject{
            { "name", shortcut.name },
            { "filePath", shortcut.filePath },
            { "target", static_cast<int>(shortcut.target) },
        });
    }

    const QJsonDocument shortcutsDoc{ shortcutsArray };
    file["shortcuts"] = QString::fromUtf8(shortcutsDoc.toJson(QJsonDocument::Compact));
    file["uuid"] = uuid;

    file["OverrideGameTime"] = gameTime.has_value();
    if (gameTime.has_value()) {
        file["ShowGameTime"] = gameTime->show;
        file["RecordGameTime"] = gameTime->record;
    }

    file["CountGameTime"] = countGameTime;

    file["OverrideCommands"] = commands.has_value();
    if (commands.has_value()) {
        file["PreLoadCommand"] = commands->preLoad;
        file["PreLaunchCommand"] = commands->preLaunch;
        file["WrapperCommand"] = commands->wrapper;
        file["PostExitCommand"] = commands->postExit;
    }

    file["OverrideConsole"] = console.has_value();
    if (console.has_value()) {
        file["ShowConsole"] = console->show;
        file["AutoCloseConsole"] = console->autoClose;
        file["ShowConsoleOnError"] = console->showOnError;
    }

    file["ManagedPack"] = managedPack.has_value();
    if (managedPack.has_value()) {
        file["ManagedPackType"] = managedPack->type;
        file["ManagedPackID"] = managedPack->id;
        file["ManagedPackName"] = managedPack->name;
        file["ManagedPackVersionID"] = managedPack->versionId;
        file["ManagedPackVersionName"] = managedPack->versionName;
        file["ManagedPackURL"] = managedPack->url;
    }

    file["Profiler"] = profiler;

    file["OverrideJavaLocation"] = javaInstallation.has_value();
    if (javaInstallation.has_value()) {
        file["JavaPath"] = javaInstallation->path;
        file["JavaSignature"] = javaInstallation->signature;
        file["JavaArchitecture"] = javaInstallation->architecture;
        file["JavaRealArchitecture"] = javaInstallation->realArchitecture;
        file["JavaVersion"] = javaInstallation->version;
        file["JavaVendor"] = javaInstallation->vendor;
        file["IgnoreJavaCompatibility"] = javaInstallation->ignoreCompatibility;
    }

    file["OverrideMemory"] = memory.has_value();
    if (memory.has_value()) {
        file["MinMemAlloc"] = memory->minAlloc;
        file["MaxMemAlloc"] = memory->maxAlloc;
        file["PermGen"] = memory->permGen;
        file["LowMemWarning"] = memory->lowMemWarning;
    }

    file["OverrideJavaArgs"] = jvmArgs.has_value();
    if (jvmArgs.has_value()) {
        file["JvmArgs"] = jvmArgs.value();
    }

    file["OverrideWindow"] = gameWindow.has_value();
    file["OverrideMiscellaneous"] = gameWindow.has_value();
    if (gameWindow.has_value()) {
        file["MinecraftWinWidth"] = gameWindow->width;
        file["MinecraftWinHeight"] = gameWindow->height;
        file["LaunchMaximized"] = gameWindow->maximized;
        file["CloseAfterLaunch"] = gameWindow->hideLauncherOnOpen;
        file["QuitAfterGameStop"] = gameWindow->quitLauncherOnClose;
    }

    file["OverrideNativeWorkarounds"] = nativeLibraries.has_value();
    if (nativeLibraries.has_value()) {
        file["UseNativeOpenAL"] = nativeLibraries->openAL;
        file["CustomOpenALPath"] = nativeLibraries->customOpenALPath;
        file["UseNativeGLFW"] = nativeLibraries->glfw;
        file["CustomGLFWPath"] = nativeLibraries->customGLFWPath;
        file["UseNativeSDL"] = nativeLibraries->sdl;
        file["CustomSDLPath"] = nativeLibraries->customSDLPath;
    }

    file["OverridePerformance"] = performance.has_value();
    if (performance.has_value()) {
        file["EnableFeralGamemode"] = performance->enableFeralGamemode;
        file["EnableMangoHud"] = performance->enableMangoHud;
        file["UseDiscreteGpu"] = performance->useDiscreteGpu;
        file["UseZink"] = performance->useZink;
    }

    file["OverrideLegacySettings"] = legacy.has_value();
    if (legacy.has_value()) {
        file["OnlineFixes"] = legacy->onlineFixes;
    }

    file["OverrideEnv"] = env.has_value();
    if (env.has_value()) {
        file["Env"] = Json::fromMap(env.value());
    }

    file["UseAccountForInstance"] = defaultAccount.has_value();
    if (defaultAccount.has_value()) {
        file["InstanceAccountId"] = defaultAccount.value();
    }

    file["JoinServerOnLaunch"] = !std::holds_alternative<std::monostate>(joinOnLaunch);
    if (const auto* server = std::get_if<ServerJoinTarget>(&joinOnLaunch)) {
        file["JoinServerOnLaunchAddress"] = server->address;
    } else if (const auto* world = std::get_if<WorldJoinTarget>(&joinOnLaunch)) {
        file["JoinWorldOnLaunch"] = world->name;
    }

    file["ExportName"] = exportName;
    file["ExportVersion"] = exportVersion;
    file["ExportSummary"] = exportSummary;
    file["ExportAuthor"] = exportAuthor;
    file["ExportOptionalFiles"] = exportOptionalFiles;
    file["ExportRecommendedRAM"] = exportRecommendedRam;

    file["GlobalDataPacksEnabled"] = globalDataPacksPath.has_value();
    if (globalDataPacksPath.has_value()) {
        file["GlobalDataPacksPath"] = globalDataPacksPath.value();
    }

    file["OverrideModDownloadLoaders"] = modDownloadLoaders.has_value();
    if (modDownloadLoaders.has_value()) {
        file["ModDownloadLoaders"] = Json::fromStringList(modDownloadLoaders.value());
    }

    file["UseLatestMinecraftVersion"] = useLatestMinecraftVersionType.has_value();
    if (useLatestMinecraftVersionType.has_value()) {
        file["UseLatestMinecraftVersionType"] = useLatestMinecraftVersionType.value();
    }

    for (auto iter = uiColumnVisibility.begin(); iter != uiColumnVisibility.end(); ++iter) {
        QJsonObject obj;
        for (auto mapIter = iter.value().begin(); mapIter != iter.value().end(); ++mapIter) {
            obj[mapIter.key()] = mapIter.value();
        }

        const QJsonDocument doc{ obj };
        file["UIColumnVisibility/" + iter.key()] = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    }

    for (auto iter = uiColumnSizes.begin(); iter != uiColumnSizes.end(); ++iter) {
        QJsonObject obj;
        for (auto mapIter = iter.value().begin(); mapIter != iter.value().end(); ++mapIter) {
            obj[mapIter.key()] = mapIter.value();
        }

        const QJsonDocument doc{ obj };
        file["UIColumnSizes/" + iter.key()] = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    }

    return file.saveFile(path);
}
