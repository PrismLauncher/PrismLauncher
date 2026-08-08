#include "GlobalConfig.h"
#include "SysInfo.h"
#include "settings/INIFile.h"

#include <QSettings>
#include <QVariant>

using namespace Qt::Literals;

std::optional<GlobalConfig> GlobalConfig::load(const QString& path)
{
    qDebug() << u"Loading global config from" << path;

    INIFile file;
    if (!file.loadFile(path)) {
        return std::nullopt;
    }

    const auto value = [&file]<typename T>(const QString& key, T def) -> T {
        const QVariant val = file.value(key);
        if (!val.isValid() || !val.canConvert<T>()) {
            return def;
        }

        if (!val.canConvert<T>()) {
            const auto expected = QMetaType::fromType<T>();
            const auto got = val.metaType();
            qWarning() << u"Global config value under" << key << u"was not of the correct type - expected" << expected.name() << u"but got"
                       << got.name();
            return def;
        }

        return val.value<T>();
    };

    GlobalConfig conf{};

    conf.iconTheme = value("IconTheme", QString());
    conf.applicationTheme = value("ApplicationTheme", QString());
    conf.backgroundCat = value("BackgroundCat", QString("Kitteh"));
    conf.lastUsedGroupForNewInstance = value("LastUsedGroupForNewInstance", QString());
    conf.menuBarInsteadOfToolBar = value("MenuBarInsteadOfToolBar", false);

    conf.numberOfConcurrentTasks = value("NumberOfConcurrentTasks", 10);
    conf.numberOfConcurrentDownloads = value("NumberOfConcurrentDownloads", 6);
    conf.numberOfManualRetries = value("NumberOfManualRetries", 1);
    conf.requestTimeout = value("RequestTimeout", 60);

    conf.consoleFont = value("ConsoleFont", QString("Courier New"));  // FIXME: don't hardcode this!
    conf.consoleFontSize = value("ConsoleFontSize", 11);              // FIXME: no hardcode!
    conf.consoleMaxLines = value("ConsoleMaxLines", 100'000);
    conf.consoleOverflowStop = value("ConsoleOverflowStop", true);

    conf.instanceDir = value("InstanceDir", QString("instances"));
    conf.additionalInstanceDirs = value("AdditionalInstanceDirs", QStringList());
    conf.lastUsedInstDirForNewInstance = value("LastUsedInstDirForNewInstance", QString());
    conf.centralModsDir = value("CentralModsDir", QString("mods"));
    conf.iconsDir = value("IconsDir", QString("icons"));
    conf.downloadsDir = value("DownloadsDir", QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    conf.downloadsDirWatchRecursive = value("DownloadsDirWatchRecursive", false);
    conf.moveModsFromDownloadsDir = value("MoveModsFromDownloadsDir", false);
    conf.skinsDir = value("SkinsDir", QString("skins"));
    conf.javaDir = value("JavaDir", QString("java"));

    conf.language = value("Language", QString());
    conf.useSystemLocale = value("UseSystemLocale", false);

    conf.console = ConsoleOverrides{
        .show = value("ShowConsole", false),
        .autoClose = value("AutoCloseConsole", false),
        .showOnError = value("ShowConsoleOnError", true),
    };

    conf.gameWindow = GameWindowOverrides{
        .maximized = value("LaunchMaximized", false),
        .width = value("MinecraftWinWidth", 854),
        .height = value("MinecraftWinHeight", 480),
        .hideLauncherOnOpen = value("CloseAfterLaunch", false),
        .quitLauncherOnClose = value("QuitAfterGameStop", false),
    };

    conf.proxyType = value("ProxyType", QString());
    conf.proxyAddr = value("ProxyAddr", QString("127.0.0.1"));
    conf.proxyPort = value("ProxyPort", 8080);
    conf.proxyUser = value("ProxyUser", QString());
    conf.proxyPass = value("ProxyPass", QString());

    // FIXME: probably should be cached per instance, but technically doesn't matter (I think)
    static const int s_defaultMaxJvmMem = SysInfo::defaultMaxJvmMem();
    conf.memory = MemoryOverrides{
        .minAlloc = value("MinMemAlloc", 512),
        .maxAlloc = value("MaxMemAlloc", s_defaultMaxJvmMem),
        .permGen = value("PermGen", 128),
        .lowMemWarning = value("LowMemWarning", true),
    };

    conf.javaInstallation = JavaInstallationOverrides{
        .path = value("JavaPath", QString()),
        .signature = value("JavaSignature", QString()),
        .architecture = value("JavaArchitecture", QString()),
        .realArchitecture = value("JavaRealArchitecture", QString()),
        .version = value("JavaVersion", QString()),
        .vendor = value("JavaVendor", QString()),
        .ignoreCompatibility = value("IgnoreJavaCompatibility", false),
    };
    conf.lastHostname = value("LastHostname", QString());
    conf.jvmArgs = value("JvmArgs", QString());
    conf.ignoreJavaWizard = value("IgnoreJavaWizard", false);
    conf.automaticJavaSwitch = value("AutomaticJavaSwitch", conf.javaInstallation.path.isEmpty());
    conf.automaticJavaDownload = value("AutomaticJavaDownload", conf.javaInstallation.path.isEmpty());
    conf.userAskedAboutAutomaticJavaDownload = value("UserAskedAboutAutomaticJavaDownload", false);

    conf.legacy = LegacyOverrides{
        .onlineFixes = value("OnlineFixes", false),
    };

    conf.nativeLibraries = NativeLibraryOverrides{
        .glfw = value("UseNativeGLFW", false),
        .customGLFWPath = value("CustomGLFWPath", QString()),
        .openAL = value("UseNativeOpenAL", false),
        .customOpenALPath = value("CustomOpenALPath", QString()),
        .sdl = value("UseNativeSDL", false),
        .customSDLPath = value("CustomSDLPath", QString()),
    };

    conf.performance = PerformanceOverrides{
        .enableFeralGamemode = value("EnableFeralGamemode", false),
        .enableMangoHud = value("EnableMangoHud", false),
        .useDiscreteGpu = value("UseDiscreteGpu", false),
        .useZink = value("UseZink", false),
    };

    conf.gameTime = GameTimeOverrides{
        .show = value("ShowGameTime", true),
        .record = value("RecordGameTime", true),
    };
    conf.showGlobalGameTime = value("ShowGlobalGameTime", true);
    conf.showGameTimeWithoutDays = value("ShowGameTimeWithoutDays", true);
    conf.totalPlayTime = int64_t(value("TotalPlayTime", qlonglong(0)));
    conf.totalPlayTimeMigrated = value("TotalPlayTimeMigrated", false);

    conf.modMetadataDisabled = value("ModMetadataDisabled", false);
    conf.modDependenciesDisabled = value("ModDependenciesDisabled", false);
    conf.skipModpackUpdatePrompt = value("SkipModpackUpdatePrompt", false);
    conf.showModIncompat = value("ShowModIncompat", false);
    conf.downloadGameFilesDuringInstanceCreation = value("DownloadGameFilesDurationInstanceCreation", true);

    conf.lastOfflinePlayerName = value("LastOfflinePlayerName", QString());

    conf.commands = CommandOverrides{
        .preLaunch = value("PreLaunchCommand", QString()),
        .wrapper = value("WrapperCommand", QString()),
        .postExit = value("PostExitCommand", QString()),
    };

    conf.enableCat = value("EnableCat", true);
    conf.theCat = value("TheCat", false);
    conf.catOpacity = value("CatOpacity", 100);
    conf.catFit = value("CatFit", QString("Fit"));

    conf.statusBarVisible = value("StatusBarVisible", true);
    conf.toolbarsLocked = value("ToolbarsLocked", false);

    conf.instSortMode = value("InstSortMode", QString("Name"));
    conf.instRenamingMode = value("InstRenamingMode", QString("AskEverytime"));
    conf.editInstanceOnDoubleClick = value("EditInstanceOnDoubleClick", false);
    conf.selectedInstance = value("SelectedInstance", QString());

    conf.pastebinType = value("PastebinType", PasteUpload::PasteType::Mclogs);
    if (conf.pastebinType < PasteUpload::PasteType::First || conf.pastebinType > PasteUpload::PasteType::Last) {
        conf.pastebinType = PasteUpload::PasteType::Mclogs;
    }

    const QString pastebinURL = value("PastebinURL", QString());
    if (!pastebinURL.isEmpty() && pastebinURL != "https://0x0.st") {
        conf.pastebinType = PasteUpload::PasteType::NullPointer;
        conf.pastebinCustomApiBase = pastebinURL;
    } else {
        conf.pastebinCustomApiBase = value("PastebinCustomAPIBase", QString());
    }

    // FIXME: no longer resets URL
    conf.metaUrlOverride = value("MetaURLOverride", QString());
    conf.resourceUrlOverride = value("ResourceURLOverride", value("ResourceURL", QString()));
    conf.legacyFmlLibsUrlOverride = value("LegacyFMLLibsURLOverride", QString());

    conf.metaRefreshOnLaunch = value("MetaRefreshOnLaunch", true);

    conf.env = Json::toMap(value("Env", QString("{}")));

    conf.msaClientIdOverride = value("MSAClientIDOverride", QString());
    conf.flameKeyOverride = value("FlameKeyOverride", value("CFKeyOverride", QString()));
    conf.fallbackModrinthBlockedMods = value("FallbackMRBlockedMods", true);
    conf.modrinthToken = value("ModrinthToken", QString());
    conf.userAgentOverride = value("UserAgentOverride", QString());
    conf.ftbAppInstancesPath = value("FTBAppInstancesPath", QString());
    conf.technicClientId = value("TechnicClientID", QString());

    conf.jsonEditorPath = value("JsonEditor", QString());
    conf.mcEditPath = value("MCEditPath", QString());
    conf.jProfilerPath = value("JProfilerPath", QString());
    conf.jProfilerPort = value("JProfilerPort", 42042);
    conf.jVisualVmPath = value("JVisualVMPath", QString());

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
        } else if (removePrefix(key, u"UIWideBarState/"_s)) {
            const auto decoded = QByteArray::fromBase64(iter.value().toByteArray());
            conf.uiWideBarState[key] = decoded;
        } else if (removePrefix(key, u"UIColumnVisibility/"_s)) {
            const auto doc = QJsonDocument::fromJson(iter.value().toByteArray());
            if (!doc.isObject()) {
                qWarning() << u"Expected JSON object under global config key" << iter.key();
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

bool GlobalConfig::save(const QString& path) const
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
    file["TotalPlayTime"] = qlonglong(totalPlayTime);
    file["TotalPlayTimeMigrated"] = totalPlayTimeMigrated;

    file["ModMetadataDisabled"] = modMetadataDisabled;
    file["ModDependenciesDisabled"] = modDependenciesDisabled;
    file["SkipModpackUpdatePrompt"] = skipModpackUpdatePrompt;
    file["ShowModIncompat"] = showModIncompat;
    file["DownloadGameFilesDuringInstanceCreation"] = downloadGameFilesDuringInstanceCreation;

    file["LastOfflinePlayerName"] = lastOfflinePlayerName;

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

    file["PastebinType"] = pastebinType;
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

    for (auto iter = uiGeometry.begin(); iter != uiGeometry.end(); ++iter) {
        file["UIGeometry/" + iter.key()] = QString::fromLatin1(iter.value().toBase64());
    }

    for (auto iter = uiState.begin(); iter != uiState.end(); ++iter) {
        file["UIState/" + iter.key()] = QString::fromLatin1(iter.value().toBase64());
    }

    for (auto iter = uiWideBarState.begin(); iter != uiWideBarState.end(); ++iter) {
        file["UIWideBarState/" + iter.key()] = QString::fromLatin1(iter.value().toBase64());
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
