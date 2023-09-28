// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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

auto ProviderCapabilities::name(ResourceProvider p) -> const char*
{
    switch (p) {
        case ResourceProvider::MODRINTH:
            return "modrinth";
        case ResourceProvider::FLAME:
            return "curseforge";
    }
    return {};
}
auto ProviderCapabilities::readableName(ResourceProvider p) -> QString
{
    switch (p) {
        case ResourceProvider::MODRINTH:
            return "Modrinth";
        case ResourceProvider::FLAME:
            return "CurseForge";
    }
    return {};
}
auto ProviderCapabilities::hashType(ResourceProvider p) -> QStringList
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

auto ProviderCapabilities::hash(ResourceProvider p, QIODevice* device, QString type) -> QString
{
    QCryptographicHash::Algorithm algo = QCryptographicHash::Sha1;
    switch (p) {
        case ResourceProvider::MODRINTH: {
            algo = (type == "sha1") ? QCryptographicHash::Sha1 : QCryptographicHash::Sha512;
            break;
        }
        case ResourceProvider::FLAME:
            algo = (type == "sha1") ? QCryptographicHash::Sha1 : QCryptographicHash::Md5;
            break;
    }

    QCryptographicHash hash(algo);
    if (!hash.addData(device))
        qCritical() << "Failed to read JAR to create hash!";

    Q_ASSERT(hash.result().length() == hash.hashLength(algo));
    return { hash.result().toHex() };
}

QString getMetaURL(ResourceProvider provider, QVariant projectID)
{
    return ((provider == ModPlatform::ResourceProvider::FLAME) ? "https://www.curseforge.com/projects/" : "https://modrinth.com/mod/") +
           projectID.toString();
}

auto getModLoaderString(ModLoaderType type) -> const QString
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

}  // namespace ModPlatform
