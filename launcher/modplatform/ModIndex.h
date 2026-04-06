// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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

#pragma once

#include <QList>
#include <QMetaType>
#include <QString>
#include <QVariant>
#include <compare>
#include <cstdint>
#include <memory>

class QIODevice;

namespace ModPlatform {

enum class ModLoaderType : std::uint16_t {
    NeoForge = 1U << 0U,
    Forge = 1U << 1U,
    Cauldron = 1U << 2U,
    LiteLoader = 1U << 3U,
    Fabric = 1U << 4U,
    Quilt = 1U << 5U,
    DataPack = 1U << 6U,
    Babric = 1U << 7U,
    BTA = 1U << 8U,
    LegacyFabric = 1U << 9U,
    Ornithe = 1U << 10U,
    Rift = 1U << 11U
};

ModLoaderType operator|(ModLoaderType lhs, ModLoaderType rhs);

using enum ModLoaderType;

Q_DECLARE_FLAGS(ModLoaderTypes, ModLoaderType)
QList<ModLoaderType> modLoaderTypesToList(ModLoaderTypes flags);

enum class ResourceProvider : std::uint8_t { MODRINTH, FLAME };

enum class DependencyType : std::uint8_t { REQUIRED, OPTIONAL, INCOMPATIBLE, EMBEDDED, TOOL, INCLUDE, UNKNOWN };

enum class Side : std::uint8_t { NoSide = 0, ClientSide = 1U << 0U, ServerSide = 1U << 1U, UniversalSide = ClientSide | ServerSide };

namespace SideUtils {
QString toString(Side side);
Side fromString(QString side);
}  // namespace SideUtils

namespace DependencyTypeUtils {
QString toString(DependencyType type);
DependencyType fromString(const QString& str);
}  // namespace DependencyTypeUtils

namespace ProviderCapabilities {
const char* name(ResourceProvider);
QString readableName(ResourceProvider);
QStringList hashType(ResourceProvider);
}  // namespace ProviderCapabilities

struct ModpackAuthor {
    QString name;
    QString url;
};

struct DonationData {
    QString id;
    QString platform;
    QString url;
};

struct IndexedVersionType {
    enum class Enum : std::uint8_t { Unknown = 0, Release = 1, Beta = 2, Alpha = 3 };
    using enum Enum;
    constexpr IndexedVersionType(Enum e = Unknown) : m_type(e) {}  // NOLINT(hicpp-explicit-conversions)
    static IndexedVersionType fromString(const QString& type);
    bool isValid() const { return m_type != Unknown; }
    std::strong_ordering operator<=>(const IndexedVersionType& other) const = default;
    std::strong_ordering operator<=>(const IndexedVersionType::Enum& other) const { return m_type <=> other; }
    QString toString() const;
    explicit operator int() const { return static_cast<int>(m_type); }
    explicit operator IndexedVersionType::Enum() { return m_type; }

   private:
    Enum m_type;
};

struct Dependency {
    QVariant addonId;
    DependencyType type;
    QString version;
};

struct IndexedVersion {
    QVariant addonId;
    QVariant fileId;
    QString version;
    QString version_number;
    IndexedVersionType version_type;
    QStringList mcVersion;
    QString downloadUrl;
    QString date;
    QString fileName;
    ModLoaderTypes loaders;
    QString hash_type;
    QString hash;
    bool is_preferred = true;
    QString changelog;
    QList<Dependency> dependencies;
    Side side = Side::NoSide;  // this is for flame API

    // For internal use, not provided by APIs
    bool is_currently_selected = false;

    QString getVersionDisplayString() const
    {
        auto release_type = version_type.isValid() ? QString(" [%1]").arg(version_type.toString()) : "";
        auto versionStr = !version.contains(version_number) ? version_number : "";
        QString gameVersion = "";
        for (const auto& v : mcVersion) {
            if (version.contains(v)) {
                gameVersion = "";
                break;
            }
            if (gameVersion.isEmpty()) {
                gameVersion = QObject::tr(" for %1").arg(v);
            }
        }
        return QString("%1%2 — %3%4").arg(version, gameVersion, versionStr, release_type);
    }
};

struct ExtraPackData {
    QList<DonationData> donate;

    QString issuesUrl;
    QString sourceUrl;
    QString wikiUrl;
    QString discordUrl;

    QString status;

    QString body;
};

struct IndexedPack {
    using Ptr = std::shared_ptr<IndexedPack>;

    QVariant addonId;
    ResourceProvider provider;
    QString name;
    QString slug;
    QString description;
    QList<ModpackAuthor> authors;
    QString logoName;
    QString logoUrl;
    QString websiteUrl;
    Side side = Side::NoSide;

    bool versionsLoaded = false;
    QList<IndexedVersion> versions;

    // Don't load by default, since some modplatform don't have that info
    bool extraDataLoaded = true;
    ExtraPackData extraData;

    // For internal use, not provided by APIs
    bool isVersionSelected(int index) const
    {
        if (!versionsLoaded) {
            return false;
        }

        return versions.at(index).is_currently_selected;
    }
    bool isAnyVersionSelected() const
    {
        if (!versionsLoaded) {
            return false;
        }

        return std::any_of(versions.constBegin(), versions.constEnd(), [](const auto& v) { return v.is_currently_selected; });
    }
};

struct OverrideDep {
    QString quilt;
    QString fabric;
    QString slug;
    ModPlatform::ResourceProvider provider;
};

inline auto getOverrideDeps() -> QList<OverrideDep>
{
    return {
        { .quilt = "634179", .fabric = "306612", .slug = "API", .provider = ModPlatform::ResourceProvider::FLAME },
        { .quilt = "720410", .fabric = "308769", .slug = "KotlinLibraries", .provider = ModPlatform::ResourceProvider::FLAME },

        { .quilt = "qvIfYCYJ", .fabric = "P7dR8mSH", .slug = "API", .provider = ModPlatform::ResourceProvider::MODRINTH },
        { .quilt = "lwVhp9o5", .fabric = "Ha28R6CL", .slug = "KotlinLibraries", .provider = ModPlatform::ResourceProvider::MODRINTH }
    };
}

QString getMetaURL(ResourceProvider provider, QVariant projectID);

auto getModLoaderAsString(ModLoaderType type) -> const QString;
auto getModLoaderFromString(QString type) -> ModLoaderType;

constexpr bool hasSingleModLoaderSelected(ModLoaderTypes l) noexcept
{
    auto x = static_cast<std::uint16_t>(l);
    return (x != 0U) && ((x & (x - 1U)) == 0U);
}

struct Category {
    QString name;
    QString id;
};

}  // namespace ModPlatform

Q_DECLARE_METATYPE(ModPlatform::IndexedPack)
Q_DECLARE_METATYPE(ModPlatform::IndexedPack::Ptr)
Q_DECLARE_METATYPE(ModPlatform::ResourceProvider)
