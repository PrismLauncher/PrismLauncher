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

#include "modplatform/ModIndex.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QIODevice>

namespace ModPlatform {

static const QMap<QString, IndexedVersionType> s_indexed_version_type_names = { { "release", IndexedVersionType::Release },
                                                                                { "beta", IndexedVersionType::Beta },
                                                                                { "alpha", IndexedVersionType::Alpha } };

static const QList<ModLoaderType> loaderList = { NeoForge, Forge, Cauldron,     LiteLoader, Quilt, Fabric,
                                                 Babric,   BTA,   LegacyFabric, Ornithe,    Rift };

QList<ModLoaderType> modLoaderTypesToList(ModLoaderTypes flags)
{
    QList<ModLoaderType> flagList;
    for (auto flag : loaderList) {
        if (flags.testFlag(flag)) {
            flagList.append(flag);
        }
    }
    return flagList;
}

QString IndexedVersionType::toString() const
{
    return s_indexed_version_type_names.key(m_type, "unknown");
}

IndexedVersionType IndexedVersionType::fromString(const QString& type)
{
    return s_indexed_version_type_names.value(type, IndexedVersionType::Unknown);
}

const char* ProviderCapabilities::name(ResourceProvider p)
{
    switch (p) {
        case ResourceProvider::MODRINTH:
            return "modrinth";
        case ResourceProvider::FLAME:
            return "curseforge";
    }
    return {};
}

QString ProviderCapabilities::readableName(ResourceProvider p)
{
    switch (p) {
        case ResourceProvider::MODRINTH:
            return "Modrinth";
        case ResourceProvider::FLAME:
            return "CurseForge";
    }
    return {};
}

QStringList ProviderCapabilities::hashType(ResourceProvider p)
{
    switch (p) {
        case ResourceProvider::MODRINTH:
            return { "sha512", "sha1" };
        case ResourceProvider::FLAME:
            // Try newer formats first, fall back to old format
            return { "sha1", "md5", "murmur2" };
    }
    return {};
}

QString getMetaURL(ResourceProvider provider, QVariant projectID)
{
    return ((provider == ModPlatform::ResourceProvider::FLAME) ? "https://www.curseforge.com/projects/" : "https://modrinth.com/mod/") +
           projectID.toString();
}

auto getModLoaderAsString(ModLoaderType type) -> const QString
{
    switch (type) {
        case NeoForge:
            return "neoforge";
        case Forge:
            return "forge";
        case Cauldron:
            return "cauldron";
        case LiteLoader:
            return "liteloader";
        case Fabric:
            return "fabric";
        case Quilt:
            return "quilt";
        case DataPack:
            return "datapack";
        case Babric:
            return "babric";
        case BTA:
            return "bta-babric";
        case LegacyFabric:
            return "legacy-fabric";
        case Ornithe:
            return "ornithe";
        case Rift:
            return "rift";
        default:
            break;
    }
    return "";
}

auto getModLoaderFromString(QString type) -> ModLoaderType
{
    if (type == "neoforge")
        return NeoForge;
    if (type == "forge")
        return Forge;
    if (type == "cauldron")
        return Cauldron;
    if (type == "liteloader")
        return LiteLoader;
    if (type == "fabric")
        return Fabric;
    if (type == "quilt")
        return Quilt;
    if (type == "babric")
        return Babric;
    if (type == "bta-babric")
        return BTA;
    if (type == "legacy-fabric")
        return LegacyFabric;
    if (type == "ornithe")
        return Ornithe;
    if (type == "rift")
        return Rift;
    return {};
}

QString SideUtils::toString(Side side)
{
    switch (side) {
        case Side::ClientSide:
            return "client";
        case Side::ServerSide:
            return "server";
        case Side::UniversalSide:
            return "both";
        case Side::NoSide:
            break;
    }
    return {};
}

Side SideUtils::fromString(QString side)
{
    if (side == "client")
        return Side::ClientSide;
    if (side == "server")
        return Side::ServerSide;
    if (side == "both")
        return Side::UniversalSide;
    return Side::UniversalSide;
}

QString DependencyTypeUtils::toString(DependencyType type)
{
    switch (type) {
        case DependencyType::REQUIRED:
            return "REQUIRED";
        case DependencyType::OPTIONAL:
            return "OPTIONAL";
        case DependencyType::INCOMPATIBLE:
            return "INCOMPATIBLE";
        case DependencyType::EMBEDDED:
            return "EMBEDDED";
        case DependencyType::TOOL:
            return "TOOL";
        case DependencyType::INCLUDE:
            return "INCLUDE";
        case DependencyType::UNKNOWN:
            return "UNKNOWN";
    }
    return "UNKNOWN";
}

DependencyType DependencyTypeUtils::fromString(const QString& str)
{
    static const QHash<QString, DependencyType> map = {
        { "REQUIRED", DependencyType::REQUIRED },
        { "OPTIONAL", DependencyType::OPTIONAL },
        { "INCOMPATIBLE", DependencyType::INCOMPATIBLE },
        { "EMBEDDED", DependencyType::EMBEDDED },
        { "TOOL", DependencyType::TOOL },
        { "INCLUDE", DependencyType::INCLUDE },
        { "UNKNOWN", DependencyType::UNKNOWN },
    };

    return map.value(str.toUpper(), DependencyType::UNKNOWN);
}
}  // namespace ModPlatform
