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

static const QMap<QString, IndexedVersionType::VersionType> s_indexed_version_type_names = {
    { "release", IndexedVersionType::VersionType::Release },
    { "beta", IndexedVersionType::VersionType::Beta },
    { "alpha", IndexedVersionType::VersionType::Alpha }
};

IndexedVersionType::IndexedVersionType(const QString& type) : IndexedVersionType(enumFromString(type)) {}

IndexedVersionType::IndexedVersionType(const IndexedVersionType::VersionType& type)
{
    m_type = type;
}

IndexedVersionType::IndexedVersionType(const IndexedVersionType& other)
{
    m_type = other.m_type;
}

IndexedVersionType& IndexedVersionType::operator=(const IndexedVersionType& other)
{
    m_type = other.m_type;
    return *this;
}

const QString IndexedVersionType::toString(const IndexedVersionType::VersionType& type)
{
    return s_indexed_version_type_names.key(type, "unknown");
}

IndexedVersionType::VersionType IndexedVersionType::enumFromString(const QString& type)
{
    return s_indexed_version_type_names.value(type, IndexedVersionType::VersionType::Unknown);
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
    return {};
}

}  // namespace ModPlatform
