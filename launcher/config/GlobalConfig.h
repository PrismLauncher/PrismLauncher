#pragma once

#include "Config.h"
#include "net/PasteUpload.h"

#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <cstdint>
#include <optional>

struct GlobalConfig {
    QString iconTheme;
    QString applicationTheme;
    QString backgroundCat;
    QString lastUsedGroupForNewInstance;
    bool menuBarInsteadOfToolBar{};

    int numberOfConcurrentTasks{};
    int numberOfConcurrentDownloads{};
    int numberOfManualRetries{};
    int requestTimeout{};

    QString consoleFont;
    int consoleFontSize{};
    int consoleMaxLines{};
    bool consoleOverflowStop{};

    QString instanceDir;
    QStringList additionalInstanceDirs;
    QString lastUsedInstDirForNewInstance;
    QString centralModsDir;
    QString iconsDir;
    QString downloadsDir;
    bool downloadsDirWatchRecursive{};
    bool moveModsFromDownloadsDir{};
    QString skinsDir;
    QString javaDir;

    // TODO: readd security-scoped bookmarks

    QString language;
    bool useSystemLocale{};

    struct ConsoleOverrides {
        bool show{};
        bool autoClose{};
        bool showOnError{};

        bool operator==(const ConsoleOverrides&) const = default;
    };
    ConsoleOverrides console;

    ConsoleOverrides consoleOrGlobal(const GlobalConfig& /*conf*/) const { return console; }

    struct GameWindowOverrides {
        bool maximized{};
        int width{};
        int height{};
        bool hideLauncherOnOpen{};
        bool quitLauncherOnClose{};

        bool operator==(const GameWindowOverrides&) const = default;
    };
    GameWindowOverrides gameWindow;

    GameWindowOverrides gameWindowOrGlobal(const GlobalConfig& /*conf*/) const { return gameWindow; }

    // TODO: switch with enum
    QString proxyType;
    QString proxyAddr;
    int proxyPort{};
    QString proxyUser;
    QString proxyPass;

    struct MemoryOverrides {
        int minAlloc{};
        int maxAlloc{};
        int permGen{};
        bool lowMemWarning{};

        bool operator==(const MemoryOverrides&) const = default;
    };
    MemoryOverrides memory;

    MemoryOverrides memoryOrGlobal(const GlobalConfig& /*conf*/) const { return memory; }

    QString jvmArgs;

    QString jvmArgsOrGlobal(const GlobalConfig& /*conf*/) const { return jvmArgs; }

    struct JavaInstallationOverrides {
        QString path;
        QString signature;
        QString architecture;
        QString realArchitecture;
        QString version;
        QString vendor;
        bool ignoreCompatibility{};

        bool operator==(const JavaInstallationOverrides&) const = default;
    };
    JavaInstallationOverrides javaInstallation;

    JavaInstallationOverrides javaInstallationOrGlobal(const GlobalConfig& /*conf*/) const { return javaInstallation; }

    QString lastHostname;
    bool ignoreJavaWizard{};
    bool automaticJavaSwitch{};
    bool automaticJavaDownload{};
    bool userAskedAboutAutomaticJavaDownload{};

    struct LegacyOverrides {
        bool onlineFixes{};

        bool operator==(const LegacyOverrides&) const = default;
    };
    LegacyOverrides legacy;

    LegacyOverrides legacyOrGlobal(const GlobalConfig& /*conf*/) const { return legacy; }

    struct NativeLibraryOverrides {
        bool glfw{};
        QString customGLFWPath;
        bool openAL{};
        QString customOpenALPath;
        bool sdl{};
        QString customSDLPath;

        bool operator==(const NativeLibraryOverrides&) const = default;
    };
    NativeLibraryOverrides nativeLibraries;

    NativeLibraryOverrides nativeLibrariesOrGlobal(const GlobalConfig& /*conf*/) const { return nativeLibraries; }

    struct PerformanceOverrides {
        bool enableFeralGamemode{};
        bool enableMangoHud{};
        bool useDiscreteGpu{};
        bool useZink{};

        bool operator==(const PerformanceOverrides&) const = default;
    };
    PerformanceOverrides performance;

    PerformanceOverrides performanceOrGlobal(const GlobalConfig& /*conf*/) const { return performance; }

    struct GameTimeOverrides {
        bool show{};
        bool record{};

        bool operator==(const GameTimeOverrides&) const = default;
    };
    GameTimeOverrides gameTime;

    GameTimeOverrides gameTimeOrGlobal(const GlobalConfig& /*conf*/) const { return gameTime; }

    bool showGlobalGameTime{};
    bool showGameTimeWithoutDays{};
    int64_t totalPlayTime{};
    bool totalPlayTimeMigrated{};

    bool modMetadataDisabled{};
    bool modDependenciesDisabled{};
    bool skipModpackUpdatePrompt{};
    bool showModIncompat{};
    bool downloadGameFilesDuringInstanceCreation{};

    QString lastOfflinePlayerName;

    struct CommandOverrides {
        QString preLaunch;
        QString wrapper;
        QString postExit;

        bool operator==(const CommandOverrides&) const = default;
    };
    CommandOverrides commands;

    CommandOverrides commandsOrGlobal(const GlobalConfig& /*conf*/) const { return commands; }

    bool enableCat{};
    bool theCat{};
    int catOpacity{};
    // TODO: switch with enum
    QString catFit;

    bool statusBarVisible{};
    bool toolbarsLocked{};

    QString instSortMode;
    QString instRenamingMode;
    bool editInstanceOnDoubleClick{};
    QString selectedInstance;

    PasteUpload::PasteType pastebinType{};
    QUrl pastebinCustomApiBase;

    QUrl metaUrlOverride;
    QUrl resourceUrlOverride;
    QUrl legacyFmlLibsUrlOverride;

    bool metaRefreshOnLaunch{};

    QVariantMap env;

    QVariantMap envOrGlobal(const GlobalConfig& /*conf*/) const { return env; }

    QString msaClientIdOverride;
    QString flameKeyOverride;
    bool fallbackModrinthBlockedMods{};
    QString modrinthToken;
    QString userAgentOverride;
    QString ftbAppInstancesPath;
    QString technicClientId;

    QString jsonEditorPath;
    QString mcEditPath;
    QString jProfilerPath;
    int jProfilerPort{};
    QString jVisualVmPath;

    QHash<QString, QByteArray> uiGeometry;
    QHash<QString, QByteArray> uiState;
    QHash<QString, QByteArray> uiWideBarState;
    QHash<QString, QHash<QString, bool>> uiColumnVisibility;

    static std::optional<GlobalConfig> load(const QString& path);

    bool save(const QString& path) const;

    bool operator==(const GlobalConfig&) const = default;
};

// NOTE: exists to allow forward declaration
class GlobalConfigHolder : public ConfigHolder<GlobalConfig> {
   public:
    using ConfigHolder::ConfigHolder;
};
