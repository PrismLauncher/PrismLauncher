#pragma once

#include <QString>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>

#include "EnumWrapper.h"

class Version;

enum class ModLoaderTypeValue : std::uint8_t {
    Invalid,
    NeoForge,
    Forge,
    LiteLoader,
    Fabric,
    Quilt,
    Babric,
    BTABabric,
    LegacyFabric,
    Ornithe,
    Rift,
    DataPack,
};

struct ModLoaderType : EnumWrapper<ModLoaderType, ModLoaderTypeValue> {
    static constexpr auto invalid() { return Invalid; }

    static constexpr auto mapping()
    {
        return std::to_array<std::pair<ModLoaderTypeValue, const char*>>({
            { NeoForge, "neoforge" },
            { Forge, "forge" },
            { LiteLoader, "liteloader" },
            { Fabric, "fabric" },
            { Quilt, "quilt" },
            { Babric, "babric" },
            { BTABabric, "bta-babric" },
            { LegacyFabric, "legacy-fabric" },
            { Ornithe, "ornithe" },
            { Rift, "rift" },
            { DataPack, "datapack" },
        });
    }

    static constexpr auto all()
    {
        std::array<ModLoaderType, mapping().size()> result;
        std::ranges::transform(mapping(), result.begin(), [](const auto& m) { return m.first; });
        return result;
    }

    QString displayName() const;

    std::optional<QString> icon() const;

    std::optional<QString> componentId() const;

    std::optional<Version> minGameVersion() const;

    using enum ModLoaderTypeValue;
    using Base = EnumWrapper<ModLoaderType, ModLoaderTypeValue>;
    using Base::Base;
};

template <>
struct std::hash<ModLoaderType> {
    std::size_t operator()(ModLoaderType type) const noexcept { return std::to_underlying(type.value()); }
};
