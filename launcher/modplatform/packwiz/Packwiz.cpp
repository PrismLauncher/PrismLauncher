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

#include "Packwiz.h"

#include <QDebug>
#include <QDir>
#include <QObject>
#include <algorithm>
#include <compare>
#include <sstream>
#include <string>

#include "FileSystem.h"
#include "StringUtils.h"

#include "Version.h"
#include "modplatform/ModIndex.h"

#include <toml++/toml.h>

namespace Packwiz {

namespace {
auto getRealIndexName(const QDir& indexDir, const QString& normalizedFname, bool shouldFindMatch = false) -> QString
{
    const QFile indexFile(indexDir.absoluteFilePath(normalizedFname));

    QString realFname = normalizedFname;
    if (!indexFile.exists()) {
        // Tries to get similar entries
        for (auto& fileName : indexDir.entryList(QDir::Filter::Files)) {
            if (QString::compare(normalizedFname, fileName, Qt::CaseInsensitive) == 0) {
                realFname = fileName;
                break;
            }
        }

        if (shouldFindMatch && (QString::compare(normalizedFname, realFname, Qt::CaseSensitive) == 0)) {
            qCritical() << "Could not find a match for a valid metadata file!";
            qCritical() << "File:" << normalizedFname;
            return {};
        }
    }

    return realFname;
}

// Helpers
auto indexFileName(const QString& modSlug) -> QString
{
    if (modSlug.endsWith(".pw.toml")) {
        return modSlug;
    }
    return QString("%1.pw.toml").arg(modSlug);
}

// Helper functions for extracting data from the TOML file
auto stringEntry(toml::table table, const QString& entryName) -> QString
{
    auto* node = table.get(StringUtils::toStdString(entryName));
    if (!node) {
        qDebug() << "Failed to read str property '" + entryName + "' in mod metadata.";
        return {};
    }

    return node->value_or("");
}

auto intEntry(toml::table table, const QString& entryName) -> int
{
    auto* node = table.get(StringUtils::toStdString(entryName));
    if (!node) {
        qDebug() << "Failed to read int property '" + entryName + "' in mod metadata.";
        return {};
    }

    return node->value_or(0);
}

bool sortMCVersions(const QString& a, const QString& b)
{
    auto cmp = Version(a) <=> Version(b);
    if (cmp == std::strong_ordering::equal) {
        return a < b;
    }
    return cmp == std::strong_ordering::less;
}

}  // namespace
auto V1::createModFormat([[maybe_unused]] const QDir& indexDir, ModPlatform::IndexedPack& modPack, ModPlatform::IndexedVersion& modVersion)
    -> Mod
{
    Mod mod;

    mod.slug = modPack.slug;
    mod.name = modPack.name;
    mod.filename = modVersion.fileName;

    if (modPack.provider == ModPlatform::ResourceProvider::FLAME) {
        mod.mode = "metadata:curseforge";
    } else {
        mod.mode = "url";
        mod.url = modVersion.downloadUrl;
    }

    mod.hashFormat = modVersion.hashType;
    mod.hash = modVersion.hash;

    mod.provider = modPack.provider;
    mod.fileId = modVersion.fileId;
    mod.projectId = modPack.addonId;
    mod.side = modVersion.side == ModPlatform::SideType::NoSide ? modPack.side : modVersion.side;
    mod.loaders = modVersion.loaders;
    mod.mcVersions = modVersion.mcVersion;
    mod.mcVersions.removeDuplicates();
    std::ranges::sort(mod.mcVersions, sortMCVersions);
    mod.releaseType = modVersion.versionType;

    mod.versionNumber = modVersion.versionNumber;
    if (mod.versionNumber.isNull()) {  // on CurseForge, there is only a version name - not a version number
        mod.versionNumber = modVersion.version;
    }

    mod.dependencies = modVersion.dependencies;
    return mod;
}

void V1::updateModIndex(const QDir& indexDir, Mod& mod)
{
    if (!mod.isValid()) {
        qCritical() << QString("Tried to update metadata of an invalid mod!");
        return;
    }

    // Ensure the corresponding mod's info exists, and create it if not

    auto normalizedFname = indexFileName(mod.slug);
    auto realFname = getRealIndexName(indexDir, normalizedFname);

    QFile indexFile(indexDir.absoluteFilePath(realFname));

    if (realFname != normalizedFname) {
        indexFile.rename(normalizedFname);
    }

    // There's already data on there!
    // TODO: We should do more stuff here, as the user is likely trying to
    // override a file. In this case, check versions and ask the user what
    // they want to do!
    if (indexFile.exists()) {
        indexFile.remove();
    } else {
        FS::ensureFilePathExists(indexFile.fileName());
    }

    toml::table update;
    switch (mod.provider) {
        case (ModPlatform::ResourceProvider::FLAME):
            if (mod.fileId.toInt() == 0 || mod.projectId.toInt() == 0) {
                qCritical() << QString("Did not write file %1 because missing information!").arg(normalizedFname);
                return;
            }
            update = toml::table{
                { "file-id", mod.fileId.toInt() },
                { "project-id", mod.projectId.toInt() },
            };
            break;
        case (ModPlatform::ResourceProvider::MODRINTH):
            if (mod.modId().toString().isEmpty() || mod.version().toString().isEmpty()) {
                qCritical() << QString("Did not write file %1 because missing information!").arg(normalizedFname);
                return;
            }
            update = toml::table{
                { "mod-id", mod.modId().toString().toStdString() },
                { "version", mod.version().toString().toStdString() },
            };
            break;
    }

    toml::array loaders;
    for (auto loader : ModPlatform::modLoaderTypesToList(mod.loaders)) {
        loaders.push_back(getModLoaderAsString(loader).toStdString());
    }
    toml::array mcVersions;
    for (const auto& version : mod.mcVersions) {
        mcVersions.push_back(version.toStdString());
    }

    if (!indexFile.open(QIODevice::ReadWrite)) {
        qCritical() << "Could not open file" << normalizedFname << "error:" << indexFile.errorString();
        return;
    }

    toml::array deps;
    for (const auto& dep : mod.dependencies) {
        auto tbl = toml::table{ { "addonId", dep.addonId.toString().toStdString() }, { "type", dep.type.toString().toStdString() } };
        if (!dep.version.isEmpty()) {
            tbl.emplace("version", dep.version.toStdString());
        }
        deps.push_back(tbl);
    }

    // Put TOML data into the file
    QTextStream inStream(&indexFile);
    {
        auto tbl = toml::table{ { "name", mod.name.toStdString() },
                                { "filename", mod.filename.toStdString() },
                                { "side", mod.side.toString().toStdString() },
                                { "x-prismlauncher-loaders", loaders },
                                { "x-prismlauncher-mc-versions", mcVersions },
                                { "x-prismlauncher-release-type", mod.releaseType.toString().toStdString() },
                                { "x-prismlauncher-version-number", mod.versionNumber.toStdString() },
                                { "x-prismlauncher-dependencies", deps },
                                { "x-prismlauncher-lock-update", mod.lockUpdate },
                                { "download",
                                  toml::table{
                                      { "mode", mod.mode.toStdString() },
                                      { "url", mod.url.toString().toStdString() },
                                      { "hash-format", mod.hashFormat.toStdString() },
                                      { "hash", mod.hash.toStdString() },
                                  } },
                                { "update", toml::table{ { ModPlatform::ProviderCapabilities::name(mod.provider), update } } } };
        std::stringstream ss;
        ss << tbl;
        inStream << QString::fromStdString(ss.str());
    }

    indexFile.flush();
    indexFile.close();
}

void V1::deleteModIndex(const QDir& indexDir, QString& modSlug)
{
    auto normalizedFname = indexFileName(modSlug);
    auto realFname = getRealIndexName(indexDir, normalizedFname);
    if (realFname.isEmpty()) {
        return;
    }

    QFile indexFile(indexDir.absoluteFilePath(realFname));

    if (!indexFile.exists()) {
        qWarning() << QString("Tried to delete non-existent mod metadata for %1!").arg(modSlug);
        return;
    }

    if (!indexFile.remove()) {
        qWarning() << QString("Failed to remove metadata for mod %1!").arg(modSlug);
    }
}

auto V1::getIndexForMod(const QDir& indexDir, const QString& slug) -> Mod
{
    Mod mod;

    auto normalizedFname = indexFileName(slug);
    auto realFname = getRealIndexName(indexDir, normalizedFname, true);
    if (realFname.isEmpty()) {
        return {};
    }

    toml::table table;
#if TOML_EXCEPTIONS
    try {
        table = toml::parse_file(StringUtils::toStdString(indexDir.absoluteFilePath(realFname)));
    } catch (const toml::parse_error& err) {
        qWarning() << QString("Could not open file %1!").arg(normalizedFname);
        qWarning() << "Reason:" << QString(err.what());
        return {};
    }
#else
    toml::parse_result result = toml::parse_file(StringUtils::toStdString(index_dir.absoluteFilePath(real_fname)));
    if (!result) {
        qWarning() << QString("Could not open file %1!").arg(normalized_fname);
        qWarning() << "Reason:" << result.error().description();
        return {};
    }
    table = result.table();
#endif

    // index_file.close();

    mod.slug = slug;

    {  // Basic info
        mod.name = stringEntry(table, "name");
        mod.filename = stringEntry(table, "filename");
        mod.side = ModPlatform::SideType::fromString(stringEntry(table, "side"));
        mod.releaseType = ModPlatform::IndexedVersionType::fromString(table["x-prismlauncher-release-type"].value_or(""));
        mod.lockUpdate = table["x-prismlauncher-lock-update"].value_or(false);
        if (auto loaders = table["x-prismlauncher-loaders"]; loaders && loaders.is_array()) {
            for (auto&& loader : *loaders.as_array()) {
                if (loader.is_string()) {
                    mod.loaders |= ModPlatform::getModLoaderFromString(QString::fromStdString(loader.as_string()->value_or("")));
                }
            }
        }
        if (auto versions = table["x-prismlauncher-mc-versions"]; versions && versions.is_array()) {
            for (auto&& version : *versions.as_array()) {
                if (version.is_string()) {
                    auto ver = QString::fromStdString(version.as_string()->value_or(""));
                    if (!ver.isEmpty()) {
                        mod.mcVersions << ver;
                    }
                }
            }
            mod.mcVersions.removeDuplicates();
            std::ranges::sort(mod.mcVersions, sortMCVersions);
        }
    }
    mod.versionNumber = table["x-prismlauncher-version-number"].value_or("");

    {  // [download] info
        auto* downloadTable = table["download"].as_table();
        if (!downloadTable) {
            qCritical() << QString("No [download] section found on mod metadata!");
            return {};
        }

        mod.mode = stringEntry(*downloadTable, "mode");
        mod.url = stringEntry(*downloadTable, "url");
        mod.hashFormat = stringEntry(*downloadTable, "hash-format");
        mod.hash = stringEntry(*downloadTable, "hash");
    }

    {  // [update] info
        using Provider = ModPlatform::ResourceProvider;

        auto updateTable = table["update"];
        if (!updateTable || !updateTable.is_table()) {
            qCritical() << QString("No [update] section found on mod metadata!");
            return {};
        }

        auto* modProviderTable = updateTable[ModPlatform::ProviderCapabilities::name(Provider::FLAME)].as_table();
        if (modProviderTable != nullptr) {
            mod.provider = Provider::FLAME;
            mod.fileId = intEntry(*modProviderTable, "file-id");
            mod.projectId = intEntry(*modProviderTable, "project-id");
        } else {
            modProviderTable = updateTable[ModPlatform::ProviderCapabilities::name(Provider::MODRINTH)].as_table();
            if (modProviderTable != nullptr) {
                mod.provider = Provider::MODRINTH;
                mod.modId() = stringEntry(*modProviderTable, "mod-id");
                mod.version() = stringEntry(*modProviderTable, "version");
            } else {
                qCritical() << QString("No mod provider on mod metadata!");
                return {};
            }
        }
    }
    {  // dependencies
        auto* deps = table["x-prismlauncher-dependencies"].as_array();
        if (deps) {
            for (auto&& depNode : *deps) {
                auto* dep = depNode.as_table();
                if (dep) {
                    ModPlatform::Dependency d;
                    d.addonId = stringEntry(*dep, "addonId");
                    if (dep->contains("version")) {
                        d.version = stringEntry(*dep, "version");
                    }
                    d.type = ModPlatform::DependencyType::fromString(stringEntry(*dep, "type"));
                    mod.dependencies << d;
                }
            }
        }
    }

    return mod;
}

auto V1::getIndexForMod(const QDir& indexDir, QVariant& modId) -> Mod
{
    for (auto& fileName : indexDir.entryList(QDir::Filter::Files)) {
        auto mod = getIndexForMod(indexDir, fileName);

        if (mod.modId() == modId) {
            return mod;
        }
    }

    return {};
}

}  // namespace Packwiz
