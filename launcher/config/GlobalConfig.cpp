// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 TheKodeToad <TheKodeToad@proton.me>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include <QFont>
#include <QFontInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>
#include <QVariant>
#include <algorithm>

#include "Application.h"
#include "GlobalConfig.h"
#include "INIFile.h"
#include "Json.h"
#include "SysInfo.h"

using namespace Qt::Literals;

namespace {
QString defaultMonospaceFont()
{
    QString defaultMonospace;
#ifdef Q_OS_WIN32
    defaultMonospace = "Courier";
#elif defined(Q_OS_MAC)
    defaultMonospace = "Menlo";
#else
    defaultMonospace = "Monospace";
#endif

    if (APPLICATION_DYN == nullptr) {
        // NOTE: this prevents an error in tests
        return defaultMonospace;
    }

    // resolve the font so the default actually matches
    QFont font;
    font.setFamily(defaultMonospace);
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);

    const QFontInfo info(font);
    qDebug().nospace() << "Detected default console font: " << info.family() << ", substitutions: " << QFont::substitutions().join(',');

    return font.family();
}

GlobalConfig loadConfig(const INIFile& file)
{
    GlobalConfig conf{};

    conf.iconTheme = file.convert<QString>("IconTheme");
    conf.applicationTheme = file.convert<QString>("ApplicationTheme");
    conf.backgroundCat = file.convert<QString>("BackgroundCat", "Kitteh");
    conf.lastUsedGroupForNewInstance = file.convert<QString>("LastUsedGroupForNewInstance");
    conf.menuBarInsteadOfToolBar = file.convert("MenuBarInsteadOfToolBar", false);

    conf.numberOfConcurrentTasks = file.convert("NumberOfConcurrentTasks", 10);
    conf.numberOfConcurrentDownloads = file.convert("NumberOfConcurrentDownloads", 6);
    conf.numberOfManualRetries = file.convert("NumberOfManualRetries", 1);
    conf.requestTimeout = file.convert("RequestTimeout", 60);

#ifdef Q_OS_WIN32
    constexpr int defaultConsoleFontSize = 10;
#else
    constexpr int defaultConsoleFontSize = 11;
#endif

    conf.consoleFont = file.convert("ConsoleFont", defaultMonospaceFont());
    conf.consoleFontSize = file.convert("ConsoleFontSize", defaultConsoleFontSize);
    conf.consoleMaxLines = std::max(file.convert("ConsoleMaxLines", 100'000), 1);
    conf.consoleOverflowStop = file.convert("ConsoleOverflowStop", true);

    conf.instanceDir = file.convert<QString>("InstanceDir", "instances");
    conf.additionalInstanceDirs = file.convert<QStringList>("AdditionalInstanceDirs");
    conf.lastUsedInstDirForNewInstance = file.convert<QString>("LastUsedInstDirForNewInstance");
    conf.centralModsDir = file.convert<QString>("CentralModsDir", "mods");
    conf.iconsDir = file.convert<QString>("IconsDir", "icons");
    conf.downloadsDir = file.convert("DownloadsDir", QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    conf.downloadsDirWatchRecursive = file.convert("DownloadsDirWatchRecursive", false);
    conf.moveModsFromDownloadsDir = file.convert("MoveModsFromDownloadsDir", false);
    conf.skinsDir = file.convert<QString>("SkinsDir", "skins");
    conf.javaDir = file.convert<QString>("JavaDir", "java");

    conf.language = file.convert<QString>("Language");
    conf.useSystemLocale = file.convert("UseSystemLocale", false);

    conf.console = GlobalConfig::ConsoleOverrides{
        .show = file.convert("ShowConsole", false),
        .autoClose = file.convert("AutoCloseConsole", false),
        .showOnError = file.convert("ShowConsoleOnError", true),
    };

    conf.gameWindow = GlobalConfig::GameWindowOverrides{
        .maximized = file.convert("LaunchMaximized", false),
        .width = file.convert("MinecraftWinWidth", 854),
        .height = file.convert("MinecraftWinHeight", 480),
        .hideLauncherOnOpen = file.convert("CloseAfterLaunch", false),
        .quitLauncherOnClose = file.convert("QuitAfterGameStop", false),
    };

    conf.proxyType = file.convert<QString>("ProxyType", "None");
    conf.proxyAddr = file.convert<QString>("ProxyAddr", "127.0.0.1");
    conf.proxyPort = file.convert("ProxyPort", 8080);
    conf.proxyUser = file.convert<QString>("ProxyUser");
    conf.proxyPass = file.convert<QString>("ProxyPass");

    const auto memoryDefaults = GlobalConfig::MemoryOverrides::defaults();
    conf.memory = GlobalConfig::MemoryOverrides{
        .minAlloc = file.convert("MinMemAlloc", memoryDefaults.minAlloc),
        .maxAlloc = file.convert("MaxMemAlloc", memoryDefaults.maxAlloc),
        .permGen = file.convert("PermGen", memoryDefaults.permGen),
        .lowMemWarning = file.convert("LowMemWarning", memoryDefaults.lowMemWarning),
    };

    conf.javaInstallation = GlobalConfig::JavaInstallationOverrides{
        .path = file.convert<QString>("JavaPath"),
        .signature = file.convert<QString>("JavaSignature"),
        .architecture = file.convert<QString>("JavaArchitecture"),
        .realArchitecture = file.convert<QString>("JavaRealArchitecture"),
        .version = file.convert<QString>("JavaVersion"),
        .vendor = file.convert<QString>("JavaVendor"),
        .ignoreCompatibility = file.convert("IgnoreJavaCompatibility", false),
    };
    conf.lastHostname = file.convert<QString>("LastHostname");
    conf.jvmArgs = file.convert<QString>("JvmArgs");
    conf.ignoreJavaWizard = file.convert("IgnoreJavaWizard", false);
    conf.automaticJavaSwitch = file.convert("AutomaticJavaSwitch", conf.javaInstallation.path.isEmpty());
    conf.automaticJavaDownload = file.convert("AutomaticJavaDownload", conf.javaInstallation.path.isEmpty());
    conf.userAskedAboutAutomaticJavaDownload = file.convert("UserAskedAboutAutomaticJavaDownload", false);

    conf.legacy = GlobalConfig::LegacyOverrides{
        .onlineFixes = file.convert("OnlineFixes", false),
    };

    conf.nativeLibraries = GlobalConfig::NativeLibraryOverrides{
        .glfw = file.convert("UseNativeGLFW", false),
        .customGLFWPath = file.convert<QString>("CustomGLFWPath"),
        .openAL = file.convert("UseNativeOpenAL", false),
        .customOpenALPath = file.convert<QString>("CustomOpenALPath"),
        .sdl = file.convert("UseNativeSDL", false),
        .customSDLPath = file.convert<QString>("CustomSDLPath"),
    };

    conf.performance = GlobalConfig::PerformanceOverrides{
        .enableFeralGamemode = file.convert("EnableFeralGamemode", false),
        .enableMangoHud = file.convert("EnableMangoHud", false),
        .useDiscreteGpu = file.convert("UseDiscreteGpu", false),
        .useZink = file.convert("UseZink", false),
    };

    conf.gameTime = GlobalConfig::GameTimeOverrides{
        .show = file.convert("ShowGameTime", true),
        .record = file.convert("RecordGameTime", true),
    };
    conf.showGlobalGameTime = file.convert("ShowGlobalGameTime", true);
    conf.showGameTimeWithoutDays = file.convert("ShowGameTimeWithoutDays", true);
    conf.totalPlayTime = file.convert<qint64>("TotalPlayTime");
    conf.totalPlayTimeMigrated = file.convert("TotalPlayTimeMigrated", false);

    conf.modMetadataDisabled = file.convert("ModMetadataDisabled", false);
    conf.modDependenciesDisabled = file.convert("ModDependenciesDisabled", false);
    conf.skipModpackUpdatePrompt = file.convert("SkipModpackUpdatePrompt", false);
    conf.showModIncompat = file.convert("ShowModIncompat", false);
    conf.downloadGameFilesDuringInstanceCreation = file.convert("DownloadGameFilesDuringInstanceCreation", true);
    conf.modUpdateReleaseTypes = Json::toStringList(file.convert<QString>("ModUpdateReleaseTypes", "[]"));

    conf.lastOfflinePlayerName = file.convert<QString>("LastOfflinePlayerName");

    conf.commands = GlobalConfig::CommandOverrides{
        .preLoad = file.convert<QString>("PreLoadCommand"),
        .preLaunch = file.convert<QString>("PreLaunchCommand"),
        .wrapper = file.convert<QString>("WrapperCommand"),
        .postExit = file.convert<QString>("PostExitCommand"),
    };

    conf.enableCat = file.convert("EnableCat", true);
    conf.theCat = file.convert("TheCat", false);
    conf.catOpacity = file.convert("CatOpacity", 100);
    conf.catFit = file.convert<QString>("CatFit", "fit");

    conf.statusBarVisible = file.convert("StatusBarVisible", true);
    conf.toolbarsLocked = file.convert("ToolbarsLocked", false);

    conf.instSortMode = file.convert<QString>("InstSortMode", "Name");
    conf.instRenamingMode = file.convert<QString>("InstRenamingMode", "AskEverytime");
    conf.editInstanceOnDoubleClick = file.convert("EditInstanceOnDoubleClick", false);
    conf.selectedInstance = file.convert<QString>("SelectedInstance");

    auto requireHttp = [](const QUrl& url){
        if (!url.isValid()) {
            return QUrl();
        }

        if (url.scheme() != "http" && url.scheme() != "https") {
            return QUrl();
        }

        return url;
    };

    conf.pastebinType = file.convert("PastebinType", PasteUpload::Type::Invalid);
    if (!conf.pastebinType.isValid()) {
        conf.pastebinType = PasteUpload::Type::Mclogs;
    }

    const auto pastebinURL = file.convert<QString>("PastebinURL");
    if (!pastebinURL.isEmpty() && pastebinURL != PasteUpload::Type(PasteUpload::Type::NullPointer).defaultBase()) {
        conf.pastebinType = PasteUpload::Type::NullPointer;
        conf.pastebinCustomApiBase = pastebinURL;
    } else {
        conf.pastebinCustomApiBase = requireHttp(file.convert<QString>("PastebinCustomAPIBase"));
    }

    conf.metaUrlOverride = requireHttp(file.convert<QString>("MetaURLOverride"));
    conf.resourceUrlOverride = requireHttp(file.convert("ResourceURLOverride", file.convert<QString>("ResourceURL")));
    conf.legacyFmlLibsUrlOverride = requireHttp(file.convert<QString>("LegacyFMLLibsURLOverride"));

    conf.metaRefreshOnLaunch = file.convert("MetaRefreshOnLaunch", true);

    conf.env = Json::toMap(file.convert<QString>("Env", "{}"));

    conf.msaClientIdOverride = file.convert<QString>("MSAClientIDOverride");
    conf.flameKeyOverride = file.convert("FlameKeyOverride", file.convert<QString>("CFKeyOverride"));
    conf.fallbackModrinthBlockedMods = file.convert("FallbackMRBlockedMods", true);
    conf.modrinthToken = file.convert<QString>("ModrinthToken");
    conf.userAgentOverride = file.convert<QString>("UserAgentOverride");
    conf.ftbAppInstancesPath = file.convert<QString>("FTBAppInstancesPath");
    conf.technicClientId = file.convert<QString>("TechnicClientID");

    conf.jsonEditorPath = file.convert<QString>("JsonEditor");
    conf.mcEditPath = file.convert<QString>("MCEditPath");
    conf.jProfilerPath = file.convert<QString>("JProfilerPath");
    conf.jProfilerPort = file.convert("JProfilerPort", 42042);
    conf.jVisualVmPath = file.convert<QString>("JVisualVMPath");
    conf.worldTools = Json::toMap(file.convert<QString>("WorldTools", "{}"));

    for (auto iter = file.begin(); iter != file.end(); ++iter) {
        QString key = iter.key();
        auto removePrefix = [](QString& key, QStringView prefix) {
            if (!key.startsWith(prefix)) {
                return false;
            }

            key = key.mid(prefix.length());
            return true;
        };

        if (removePrefix(key, u"UIGeometry/"_s)) {
            const auto decoded = QByteArray::fromBase64(iter.value().toByteArray());
            conf.uiGeometry[key] = decoded;
        } else if (removePrefix(key, u"UIState/"_s)) {
            const auto decoded = QByteArray::fromBase64(iter.value().toByteArray());
            conf.uiState[key] = decoded;
        } else if (removePrefix(key, u"UIColumnVisibility/"_s)) {
            const auto doc = QJsonDocument::fromJson(iter.value().toByteArray());
            if (!doc.isObject()) {
                qWarning() << u"Expected JSON object for global column visibility at" << iter.key();
                continue;
            }

            const auto obj = doc.object();
            QHash<QString, bool> map;

            for (auto objIter = obj.begin(); objIter != obj.end(); ++objIter) {
                map[objIter.key()] = objIter.value().toBool();
            }

            conf.uiColumnVisibility[key] = std::move(map);
        }
    }

    return conf;
}
}  // namespace

Result<GlobalConfig> GlobalConfig::load(const QString& path)
{
    qDebug() << u"Loading global config from" << path;

    INIFile file;
    TRY(file.loadFile(path));

    return loadConfig(file);
}

GlobalConfig GlobalConfig::loadDefaults()
{
    return loadConfig(INIFile{});
}

Result<> GlobalConfig::save(const QString& path) const
{
    qDebug() << u"Saving global config to" << path;

    INIFile file;

    file["IconTheme"] = iconTheme;
    file["ApplicationTheme"] = applicationTheme;
    file["BackgroundCat"] = backgroundCat;
    file["LastUsedGroupForNewInstance"] = lastUsedGroupForNewInstance;
    file["MenuBarInsteadOfToolBar"] = menuBarInsteadOfToolBar;

    file["NumberOfConcurrentTasks"] = numberOfConcurrentTasks;
    file["NumberOfConcurrentDownloads"] = numberOfConcurrentDownloads;
    file["NumberOfManualRetries"] = numberOfManualRetries;
    file["RequestTimeout"] = requestTimeout;

    file["ConsoleFont"] = consoleFont;
    file["ConsoleFontSize"] = consoleFontSize;
    file["ConsoleMaxLines"] = consoleMaxLines;
    file["ConsoleOverflowStop"] = consoleOverflowStop;

    file["InstanceDir"] = instanceDir;
    file["AdditionalInstanceDirs"] = additionalInstanceDirs;
    file["LastUsedInstDirForNewInstance"] = lastUsedInstDirForNewInstance;
    file["CentralModsDir"] = centralModsDir;
    file["IconsDir"] = iconsDir;
    file["DownloadsDir"] = downloadsDir;
    file["DownloadsDirWatchRecursive"] = downloadsDirWatchRecursive;
    file["MoveModsFromDownloadsDir"] = moveModsFromDownloadsDir;
    file["SkinsDir"] = skinsDir;
    file["JavaDir"] = javaDir;

    file["Language"] = language;
    file["UseSystemLocale"] = useSystemLocale;

    file["ShowConsole"] = console.show;
    file["AutoCloseConsole"] = console.autoClose;
    file["ShowConsoleOnError"] = console.showOnError;

    file["LaunchMaximized"] = gameWindow.maximized;
    file["MinecraftWinWidth"] = gameWindow.width;
    file["MinecraftWinHeight"] = gameWindow.height;
    file["CloseAfterLaunch"] = gameWindow.hideLauncherOnOpen;
    file["QuitAfterGameStop"] = gameWindow.quitLauncherOnClose;

    file["ProxyType"] = proxyType;
    file["ProxyAddr"] = proxyAddr;
    file["ProxyPort"] = proxyPort;
    file["ProxyUser"] = proxyUser;
    file["ProxyPass"] = proxyPass;

    file["MinMemAlloc"] = memory.minAlloc;
    file["MaxMemAlloc"] = memory.maxAlloc;
    file["PermGen"] = memory.permGen;
    file["LowMemWarning"] = memory.lowMemWarning;

    file["JavaPath"] = javaInstallation.path;
    file["JavaSignature"] = javaInstallation.signature;
    file["JavaArchitecture"] = javaInstallation.architecture;
    file["JavaRealArchitecture"] = javaInstallation.realArchitecture;
    file["JavaVersion"] = javaInstallation.version;
    file["JavaVendor"] = javaInstallation.vendor;
    file["IgnoreJavaCompatibility"] = javaInstallation.ignoreCompatibility;

    file["LastHostname"] = lastHostname;
    file["JvmArgs"] = jvmArgs;
    file["IgnoreJavaWizard"] = ignoreJavaWizard;
    file["AutomaticJavaSwitch"] = automaticJavaSwitch;
    file["AutomaticJavaDownload"] = automaticJavaDownload;
    file["UserAskedAboutAutomaticJavaDownload"] = userAskedAboutAutomaticJavaDownload;

    file["OnlineFixes"] = legacy.onlineFixes;

    file["UseNativeGLFW"] = nativeLibraries.glfw;
    file["CustomGLFWPath"] = nativeLibraries.customGLFWPath;
    file["UseNativeOpenAL"] = nativeLibraries.openAL;
    file["CustomOpenALPath"] = nativeLibraries.customOpenALPath;
    file["UseNativeSDL"] = nativeLibraries.sdl;
    file["CustomSDLPath"] = nativeLibraries.customSDLPath;

    file["EnableFeralGamemode"] = performance.enableFeralGamemode;
    file["EnableMangoHud"] = performance.enableMangoHud;
    file["UseDiscreteGpu"] = performance.useDiscreteGpu;
    file["UseZink"] = performance.useZink;

    file["ShowGameTime"] = gameTime.show;
    file["RecordGameTime"] = gameTime.record;

    file["ShowGlobalGameTime"] = showGlobalGameTime;
    file["ShowGameTimeWithoutDays"] = showGameTimeWithoutDays;
    file["TotalPlayTime"] = qint64{ totalPlayTime };
    file["TotalPlayTimeMigrated"] = totalPlayTimeMigrated;

    file["ModMetadataDisabled"] = modMetadataDisabled;
    file["ModDependenciesDisabled"] = modDependenciesDisabled;
    file["SkipModpackUpdatePrompt"] = skipModpackUpdatePrompt;
    file["ShowModIncompat"] = showModIncompat;
    file["DownloadGameFilesDuringInstanceCreation"] = downloadGameFilesDuringInstanceCreation;
    file["ModUpdateReleaseTypes"] = Json::fromStringList(modUpdateReleaseTypes);

    file["LastOfflinePlayerName"] = lastOfflinePlayerName;

    file["PreLoadCommand"] = commands.preLoad;
    file["PreLaunchCommand"] = commands.preLaunch;
    file["WrapperCommand"] = commands.wrapper;
    file["PostExitCommand"] = commands.postExit;

    file["EnableCat"] = enableCat;
    file["TheCat"] = theCat;
    file["CatOpacity"] = catOpacity;
    file["CatFit"] = catFit;

    file["StatusBarVisible"] = statusBarVisible;
    file["ToolbarsLocked"] = toolbarsLocked;

    file["InstSortMode"] = instSortMode;
    file["InstRenamingMode"] = instRenamingMode;
    file["EditInstanceOnDoubleClick"] = editInstanceOnDoubleClick;
    file["SelectedInstance"] = selectedInstance;

    file["PastebinType"] = pastebinType.toInt();
    file["PastebinCustomAPIBase"] = pastebinCustomApiBase.toString();

    file["MetaURLOverride"] = metaUrlOverride.toString();
    file["ResourceURLOverride"] = resourceUrlOverride.toString();
    file["LegacyFMLLibsURLOverride"] = legacyFmlLibsUrlOverride.toString();

    file["MetaRefreshOnLaunch"] = metaRefreshOnLaunch;

    file["Env"] = Json::fromMap(env);

    file["MSAClientIDOverride"] = msaClientIdOverride;
    file["FlameKeyOverride"] = flameKeyOverride;
    file["FallbackMRBlockedMods"] = fallbackModrinthBlockedMods;
    file["ModrinthToken"] = modrinthToken;
    file["UserAgentOverride"] = userAgentOverride;
    file["FTBAppInstancesPath"] = ftbAppInstancesPath;
    file["TechnicClientID"] = technicClientId;

    file["JsonEditor"] = jsonEditorPath;
    file["MCEditPath"] = mcEditPath;
    file["JProfilerPath"] = jProfilerPath;
    file["JProfilerPort"] = jProfilerPort;
    file["JVisualVMPath"] = jVisualVmPath;
    file["WorldTools"] = Json::fromMap(worldTools);

    for (auto iter = uiGeometry.begin(); iter != uiGeometry.end(); ++iter) {
        file["UIGeometry/" + iter.key()] = QString::fromLatin1(iter.value().toBase64());
    }

    for (auto iter = uiState.begin(); iter != uiState.end(); ++iter) {
        file["UIState/" + iter.key()] = QString::fromLatin1(iter.value().toBase64());
    }

    for (auto iter = uiColumnVisibility.begin(); iter != uiColumnVisibility.end(); ++iter) {
        QJsonObject obj;
        for (auto mapIter = iter.value().begin(); mapIter != iter.value().end(); ++mapIter) {
            obj[mapIter.key()] = mapIter.value();
        }

        const QJsonDocument doc{ obj };
        file["UIColumnVisibility/" + iter.key()] = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    }

    return file.saveFile(path);
}

GlobalConfig::MemoryOverrides GlobalConfig::MemoryOverrides::defaults()
{
    static const int s_defaultMaxJvmMem = SysInfo::defaultMaxJvmMem();
    return {
        .minAlloc = 512,
        .maxAlloc = s_defaultMaxJvmMem,
        .permGen = 128,
        .lowMemWarning = true,

    };
}
