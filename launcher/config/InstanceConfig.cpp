#include "InstanceConfig.h"
#include "Json.h"
#include "settings/INIFile.h"

#include <algorithm>

using namespace Qt::Literals;

std::optional<InstanceConfig> InstanceConfig::load(const QString& path)
{
    qDebug() << u"Loading instance config from" << path;

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
            qWarning() << u"Instance config value under" << key << u"was not of the correct type - expected" << expected.name()
                       << u"but got" << got.name();
            return def;
        }

        return val.value<T>();
    };

    QString type = value("type", QString(""));
    if (!type.isEmpty() && type != "OneSix") {
        qWarning() << u"Bad instance type:" << type;
        return std::nullopt;
    }

    InstanceConfig conf{};

    // NOTE: all new keys should be added with UpperCamelCase for consistency
    // The lowerCamelCase keys are retained purely so that older launcher versions can understand the file format

    conf.name = value("name", QString("Unnamed Instance"));
    conf.iconKey = value("iconKey", QString("default"));
    conf.notes = value("notes", QString());

    conf.lastLaunchTime = int64_t(value("lastLaunchTime", qlonglong(0)));
    conf.totalTimePlayed = int64_t(value("totalTimePlayed", qlonglong(0)));
    conf.totalTimePlayed = std::max<int64_t>(conf.totalTimePlayed, 0);
    conf.lastTimePlayed = int64_t(value("lastTimePlayed", qlonglong(0)));

    conf.linkedInstances = Json::toStringList(value("linkedInstances", QString("[]")));

    const auto shortcutsJson = value("shortcuts", QByteArray("[]"));
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
            if (targetVal < 0 || targetVal >= int(ShortcutTarget::Count)) {
                qWarning() << u"Found invalid instance shortcut target type";
                continue;
            }

            conf.shortcuts.append(Shortcut{
                .name = shortcutName.toString(),
                .filePath = filePath.toString(),
                .target = ShortcutTarget(target.toInt()),
            });
        }
    } else {
        qWarning() << u"Expected JSON array under instance shortcuts";
    }

    conf.uuid = value("uuid", QString());

    bool overrideGameTime = value("OverrideGameTime", false);
    if (overrideGameTime) {
        conf.gameTime = GlobalConfig::GameTimeOverrides{
            .show = value("ShowGameTime", true),
            .record = value("RecordGameTime", true),
        };
    }

    conf.countGameTime = value("CountGameTime", true);

    bool overrideCommands = value("OverrideCommands", false);
    if (overrideCommands) {
        conf.commands = GlobalConfig::CommandOverrides{
            .preLaunch = value("PreLaunchCommand", QString()),
            .wrapper = value("WrapperCommand", QString()),
            .postExit = value("PostExitCommand", QString()),
        };
    }

    bool overrideConsole = value("OverrideConsole", false);
    if (overrideConsole) {
        conf.console = GlobalConfig::ConsoleOverrides{
            .show = value("ShowConsole", false),
            .autoClose = value("AutoCloseConsole", false),
            .showOnError = value("ShowConsoleOnError", true),
        };
    }

    bool isManagedPack = value("ManagedPack", false);
    if (isManagedPack) {
        conf.managedPack = ManagedPack{
            .type = value("ManagedPackType", QString()),
            .id = value("ManagedPackID", QString()),
            .name = value("ManagedPackName", QString()),
            .versionId = value("ManagedPackVersionID", QString()),
            .versionName = value("ManagedPackVersionID", QString()),
            .url = value("ManagedPackURL", QString()),
        };
    }

    conf.profiler = value("Profiler", QString());

    const auto overrideJavaInstallation = value("OverrideJavaLocation", false);
    if (overrideJavaInstallation) {
        conf.javaInstallation = GlobalConfig::JavaInstallationOverrides{
            .path = value("JavaPath", QString()),
            .signature = value("JavaSignature", QString()),
            .architecture = value("JavaArchitecture", QString()),
            .realArchitecture = value("JavaRealArchitecture", QString()),
            .version = value("JavaVersion", QString()),
            .vendor = value("JavaVendor", QString()),
            .ignoreCompatibility = value("IgnoreJavaCompatibility", false),
        };
    }

    conf.automaticJava = value("AutomaticJava", false);

    // NOTE: it was previously two group boxes, then merged into one
    const auto overrideWindow = value("OverrideWindow", false) || value("OverrideMiscellaneous", false);
    if (overrideWindow) {
        conf.gameWindow = GlobalConfig::GameWindowOverrides{
            .maximized = value("LaunchMaximized", true),
            .width = value("MinecraftWinWidth", 854),
            .height = value("MinecraftWinHeight", 480),
            .hideLauncherOnOpen = value("CloseAfterLaunch", false),
            .quitLauncherOnClose = value("QuitAfterGameStop", false),
        };
    }

    const auto overrideMemory = value("OverrideMemory", false);
    if (overrideMemory) {
        conf.memory = GlobalConfig::MemoryOverrides{
            .minAlloc = value("MinMemAlloc", 0),
            .maxAlloc = value("MaxMemAlloc", 0),
            .permGen = value("PermGen", 0),
            .lowMemWarning = value("LowMemWarning", true),
        };
    }

    const auto overrideJvmArgs = value("OverrideJavaArgs", false);
    if (overrideJvmArgs) {
        conf.jvmArgs = value("JvmArgs", QString());
    }

    const auto overrideNativeLibraries = value("OverrideNativeWorkarounds", false);
    if (overrideNativeLibraries) {
        conf.nativeLibraries = GlobalConfig::NativeLibraryOverrides{
            .glfw = value("UseNativeGLFW", false),
            .customGLFWPath = value("CustomGLFWPath", QString()),
            .openAL = value("UseNativeOpenAL", false),
            .customOpenALPath = value("CustomOpenALPath", QString()),
            .sdl = value("UseNativeSDL", false),
            .customSDLPath = value("CustomSDLPath", QString()),
        };
    }

    const auto overridePerformance = value("OverridePerformance", false);
    if (overridePerformance) {
        conf.performance = GlobalConfig::PerformanceOverrides{
            .enableFeralGamemode = value("EnableFeralGamemode", false),
            .enableMangoHud = value("EnableMangoHud", false),
            .useDiscreteGpu = value("UseDiscreteGpu", false),
            .useZink = value("UseZink", false),
        };
    }

    const auto overrideLegacySettings = value("OverrideLegacySettings", false);
    if (overrideLegacySettings) {
        conf.legacy = GlobalConfig::LegacyOverrides{
            .onlineFixes = value("OnlineFixes", false),
        };
    }

    const auto overrideEnv = value("OverrideEnv", false);
    if (overrideEnv) {
        conf.env = Json::toMap(value("Env", QString()));
    }

    const auto overrideDefaultAccount = value("UseAccountForInstance", false);
    if (overrideDefaultAccount) {
        conf.defaultAccount = value("InstanceAccountId", QString());
    }

    const auto autoJoinEnabled = value("JoinServerOnLaunch", false);
    if (autoJoinEnabled) {
        if (const auto serverAddr = value("JoinServerOnLaunchAddress", QString()); !serverAddr.isEmpty()) {
            conf.joinOnLaunch = ServerJoinTarget{ serverAddr };
        } else if (const auto world = value("JoinWorldOnLaunch", QString()); !world.isEmpty()) {
            conf.joinOnLaunch = WorldJoinTarget{ serverAddr };
        }
    }

    conf.exportName = value("ExportName", QString());
    conf.exportVersion = value("ExportVersion", QString());
    conf.exportSummary = value("ExportSummary", QString());
    conf.exportAuthor = value("ExportAuthor", QString());
    conf.exportOptionalFiles = value("ExportOptionalFiles", true);
    conf.exportRecommendedRam = value("ExportRecommendedRAM", 0);

    const auto globalDataPacksEnabled = value("GlobalDataPacksEnabled", false);
    if (globalDataPacksEnabled) {
        conf.globalDataPacksPath = value("GlobalDataPacksPath", QString());
    } else {
        conf.globalDataPacksPath = std::nullopt;
    }

    const auto overrideModDownloadLoaders = value("OverrideModDownloadLoaders", false);
    if (overrideModDownloadLoaders) {
        conf.modDownloadLoaders = Json::toStringList(value("ModDownloadLoaders", QString("[]")));
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
                qWarning() << u"Expected JSON object under global config key" << iter.key();
                continue;
            }

            const auto obj = doc.object();
            QHash<QString, bool> map;

            for (auto objIter = obj.begin(); objIter != obj.end(); ++objIter) {
                map[objIter.key()] = objIter.value().toBool();
            }

            conf.uiColumnVisibility[key] = std::move(map);
        } else if (removePrefix(key, u"UIColumnState/")) {
            const auto decoded = QByteArray::fromBase64(iter.value().toByteArray());
            conf.uiColumnState[key] = decoded;
        }
    }

    return conf;
}

bool InstanceConfig::save(const QString& path) const
{
    qDebug() << u"Saving instance config to" << path;

    INIFile file;
    file["type"] = "OneSix";

    file["name"] = name;
    file["iconKey"] = iconKey;
    file["notes"] = notes;

    file["lastLaunchTime"] = qlonglong(lastLaunchTime);
    file["totalTimePlayed"] = qlonglong(totalTimePlayed);
    file["lastTimePlayed"] = qlonglong(lastTimePlayed);

    file["linkedInstance"] = Json::fromStringList(linkedInstances);

    QJsonArray shortcutsArray;
    for (const auto& shortcut : shortcuts) {
        shortcutsArray.append(QJsonObject{
            { "name", shortcut.name },
            { "filePath", shortcut.filePath },
            { "target", int(shortcut.target) },
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
        file["PreLaunchCommand"] = commands->preLaunch;
        file["WrapperCommand"] = commands->wrapper;
        file["PostExitCommand"] = commands->postExit;
    }

    file["ConsoleOverrides"] = console.has_value();
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
        file["ModDownloadLoaders"] = modDownloadLoaders.value();
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
