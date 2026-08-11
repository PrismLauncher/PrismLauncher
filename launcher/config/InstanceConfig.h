#pragma once

#include "GlobalConfig.h"
#include "Config.h"

#include <QString>
#include <cstdint>
#include <optional>

struct InstanceConfig {
    QString name;
    QString iconKey;
    QString notes;

    int64_t lastLaunchTime{};
    int64_t totalTimePlayed{};
    int64_t lastTimePlayed{};

    QStringList linkedInstances;
    enum class ShortcutTarget : std::uint8_t { Desktop, Applications, Other, Count };
    struct Shortcut {
        QString name;
        QString filePath;
        ShortcutTarget target;

        bool operator==(const Shortcut&) const = default;
    };
    QList<Shortcut> shortcuts;
    QString uuid;

    std::optional<GlobalConfig::GameTimeOverrides> gameTime;
    bool countGameTime{};

    GlobalConfig::GameTimeOverrides gameTimeOrGlobal(const GlobalConfig& conf) const { return gameTime.value_or(conf.gameTime); }

    std::optional<GlobalConfig::CommandOverrides> commands;

    GlobalConfig::CommandOverrides commandsOrGlobal(const GlobalConfig& conf) const { return commands.value_or(conf.commands); }

    std::optional<GlobalConfig::ConsoleOverrides> console;

    GlobalConfig::ConsoleOverrides consoleOrGlobal(const GlobalConfig& conf) const { return console.value_or(conf.console); }

    struct ManagedPack {
        QString type;
        QString id;
        QString name;
        QString versionId;
        QString versionName;
        QString url;

        bool operator==(const ManagedPack&) const = default;
    };
    std::optional<ManagedPack> managedPack;

    QString profiler;

    std::optional<GlobalConfig::JavaInstallationOverrides> javaInstallation;

    GlobalConfig::JavaInstallationOverrides javaInstallationOrGlobal(const GlobalConfig& conf) const
    {
        return javaInstallation.value_or(conf.javaInstallation);
    }

    bool automaticJava;

    std::optional<GlobalConfig::MemoryOverrides> memory;

    GlobalConfig::MemoryOverrides memoryOrGlobal(const GlobalConfig& conf) const { return memory.value_or(conf.memory); }

    std::optional<QString> jvmArgs;

    QString jvmArgsOrGlobal(const GlobalConfig& conf) const { return jvmArgs.value_or(conf.jvmArgs); }

    std::optional<GlobalConfig::GameWindowOverrides> gameWindow;

    GlobalConfig::GameWindowOverrides gameWindowOrGlobal(const GlobalConfig& conf) const { return gameWindow.value_or(conf.gameWindow); }

    std::optional<GlobalConfig::NativeLibraryOverrides> nativeLibraries;

    GlobalConfig::NativeLibraryOverrides nativeLibrariesOrGlobal(const GlobalConfig& conf) const
    {
        return nativeLibraries.value_or(conf.nativeLibraries);
    }

    std::optional<GlobalConfig::PerformanceOverrides> performance;

    GlobalConfig::PerformanceOverrides performanceOrGlobal(const GlobalConfig& conf) const
    {
        return performance.value_or(conf.performance);
    }

    std::optional<GlobalConfig::LegacyOverrides> legacy;

    GlobalConfig::LegacyOverrides legacyOrGlobal(const GlobalConfig& conf) const { return legacy.value_or(conf.legacy); }

    std::optional<QVariantMap> env;

    QVariantMap envOrGlobal(const GlobalConfig& conf) const { return env.value_or(conf.env); }

    std::optional<QString> defaultAccount;

    struct ServerJoinTarget {
        QString address;

        bool operator==(const ServerJoinTarget&) const = default;
    };

    struct WorldJoinTarget {
        QString name;

        bool operator==(const WorldJoinTarget&) const = default;
    };

    std::variant<std::monostate, ServerJoinTarget, WorldJoinTarget> joinOnLaunch;

    QString exportName;
    QString exportVersion;
    QString exportSummary;
    QString exportAuthor;
    bool exportOptionalFiles{};
    int exportRecommendedRam{};

    std::optional<QString> globalDataPacksPath;

    std::optional<QStringList> modDownloadLoaders;

    QHash<QString, QHash<QString, bool>> uiColumnVisibility;
    QHash<QString, QByteArray> uiColumnState;

    static std::optional<InstanceConfig> load(const QString& path);

    bool save(const QString& path) const;

    bool operator==(const InstanceConfig&) const = default;
};

// NOTE: exists to allow forward declaration
class InstanceConfigHolder : public ConfigHolder<InstanceConfig> {
   public:
    using ConfigHolder::ConfigHolder;
};
