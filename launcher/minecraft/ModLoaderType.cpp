#include "ModLoaderType.h"
#include <qassert.h>

#include <QString>
#include <QtAssert>
#include <optional>

namespace {
struct ModLoaderInfo {
    QString displayName;
    std::optional<QString> icon;
    std::optional<QString> componentId;
    std::optional<Version> minGameVersion;
};

const auto g_modLoaderInfoMappings = std::to_array<std::pair<ModLoaderTypeValue, ModLoaderInfo>>({
    {
        ModLoaderType::NeoForge,
        {
            .displayName = "NeoForge",
            .icon = "neoforged",
            .componentId = "net.neoforged",
            .minGameVersion = {},
        },
    },
    {
        ModLoaderType::Forge,
        {
            .displayName = "Forge",
            .icon = "forge",
            .componentId = "net.minecraftforge",
            .minGameVersion = {},
        },
    },
    {
        ModLoaderType::LiteLoader,
        {
            .displayName = "LiteLoader",
            .icon = "liteloader",
            .componentId = "com.mumfrey.liteloader",
            .minGameVersion = {},
        },
    },
    {
        ModLoaderType::Fabric,
        {
            .displayName = "Fabric",
            .icon = "fabricmc",
            .componentId = "net.fabricmc.fabric-loader",
            .minGameVersion = Version("1.14"),
        },
    },
    {
        ModLoaderType::Quilt,
        {
            .displayName = "Quilt",
            .icon = "quiltmc",
            .componentId = "org.quiltmc.quilt-loader",
            .minGameVersion = Version("1.14"),
        },
    },
    {
        ModLoaderType::Babric,
        {
            .displayName = "Babric",
            .icon = std::nullopt,
            .componentId = std::nullopt,
            .minGameVersion = {},
        },
    },
    {
        ModLoaderType::BTABabric,
        {
            .displayName = "BTA (Babric)",
            .icon = std::nullopt,
            .componentId = std::nullopt,
            .minGameVersion = {},
        },
    },
    {
        ModLoaderType::LegacyFabric,
        {
            .displayName = "Legacy Fabric",
            .icon = std::nullopt,
            .componentId = std::nullopt,
            .minGameVersion = {},
        },
    },
    {
        ModLoaderType::Ornithe,
        {
            .displayName = "Ornithe",
            .icon = std::nullopt,
            .componentId = std::nullopt,
            .minGameVersion = {},
        },
    },
    {
        ModLoaderType::Rift,
        {
            .displayName = "Rift",
            .icon = std::nullopt,
            .componentId = std::nullopt,
            .minGameVersion = {},
        },
    },
    {
        ModLoaderType::DataPack,
        {
            .displayName = "Data Pack",
            .icon = std::nullopt,
            .componentId = std::nullopt,
            .minGameVersion = {},
        },
    },
});

ModLoaderInfo findModLoaderInfo(ModLoaderTypeValue val)
{
    for (const auto& mapping : g_modLoaderInfoMappings) {
        if (mapping.first == val) {
            return mapping.second;
        }
    }

    Q_ASSERT_X(false, "findModLoaderInfo", "No ModLoaderInfo mapping found for the current enum");
    return {};
}
};  // namespace

QString ModLoaderType::displayName() const
{
    return findModLoaderInfo(value()).displayName;
}

std::optional<QString> ModLoaderType::icon() const
{
    return findModLoaderInfo(value()).icon;
}

std::optional<QString> ModLoaderType::componentId() const
{
    return findModLoaderInfo(value()).componentId;
}

std::optional<Version> ModLoaderType::minGameVersion() const
{
    return findModLoaderInfo(value()).minGameVersion;
}
