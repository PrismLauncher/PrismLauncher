// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
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

#include "Packwiz.h"

#include <QDebug>
#include <QDir>
#include <QObject>

#include <toml++/toml.h>
#include "minecraft/mod/Mod.h"
#include "modplatform/ModIndex.h"

namespace Packwiz {

auto getRealIndexName(QDir& index_dir, QString normalized_fname, bool should_find_match) -> QString
{
    QFile index_file(index_dir.absoluteFilePath(normalized_fname));

    QString real_fname = normalized_fname;
    if (!index_file.exists()) {
        // Tries to get similar entries
        for (auto& file_name : index_dir.entryList(QDir::Filter::Files)) {
            if (!QString::compare(normalized_fname, file_name, Qt::CaseInsensitive)) {
                real_fname = file_name;
                break;
            }
        }

        if (should_find_match && !QString::compare(normalized_fname, real_fname, Qt::CaseSensitive)) {
            qCritical() << "Could not find a match for a valid metadata file!";
            qCritical() << "File: " << normalized_fname;
            return {};
        }
    }

    return real_fname;
}

// Helpers
static inline auto indexFileName(QString const& mod_slug) -> QString
{
    if (mod_slug.endsWith(".pw.toml"))
        return mod_slug;
    return QString("%1.pw.toml").arg(mod_slug);
}

static ModPlatform::ProviderCapabilities ProviderCaps;

// Helper functions for extracting data from the TOML file
auto stringEntry(toml::table table, const std::string entry_name) -> QString
{
    auto node = table[entry_name];
    if (!node) {
        qCritical() << QString::fromStdString("Failed to read str property '" + entry_name + "' in mod metadata.");
        return {};
    }

    return QString::fromStdString(node.value_or(""));
}

auto intEntry(toml::table table, const std::string entry_name) -> int
{
    auto node = table[entry_name];
    if (!node) {
        qCritical() << QString::fromStdString("Failed to read int property '" + entry_name + "' in mod metadata.");
        return {};
    }

    return node.value_or(0);
}

auto V1::createModFormat(QDir& index_dir, ModPlatform::IndexedPack& mod_pack, ModPlatform::IndexedVersion& mod_version) -> Mod
{
    Mod mod;

    mod.slug = mod_pack.slug;
    mod.name = mod_pack.name;
    mod.filename = mod_version.fileName;

    if (mod_pack.provider == ModPlatform::Provider::FLAME) {
        mod.mode = "metadata:curseforge";
    } else {
        mod.mode = "url";
        mod.url = mod_version.downloadUrl;
    }

    mod.hash_format = mod_version.hash_type;
    mod.hash = mod_version.hash;

    mod.provider = mod_pack.provider;
    mod.file_id = mod_version.fileId;
    mod.project_id = mod_pack.addonId;

    return mod;
}

auto V1::createModFormat(QDir& index_dir, ::Mod& internal_mod, QString slug) -> Mod
{
    // Try getting metadata if it exists
    Mod mod{ getIndexForMod(index_dir, slug) };
    if (mod.isValid())
        return mod;

    qWarning() << QString("Tried to create mod metadata with a Mod without metadata!");

    return {};
}

void V1::updateModIndex(QDir& index_dir, Mod& mod)
{
    if (!mod.isValid()) {
        qCritical() << QString("Tried to update metadata of an invalid mod!");
        return;
    }

    // Ensure the corresponding mod's info exists, and create it if not

    auto normalized_fname = indexFileName(mod.slug);
    auto real_fname = getRealIndexName(index_dir, normalized_fname);

    QFile index_file(index_dir.absoluteFilePath(real_fname));

    if (real_fname != normalized_fname)
        index_file.rename(normalized_fname);

    // There's already data on there!
    // TODO: We should do more stuff here, as the user is likely trying to
    // override a file. In this case, check versions and ask the user what
    // they want to do!
    if (index_file.exists()) {
        index_file.remove();
    }

    if (!index_file.open(QIODevice::ReadWrite)) {
        qCritical() << QString("Could not open file %1!").arg(indexFileName(mod.name));
        return;
    }

    // Put TOML data into the file
    QTextStream in_stream(&index_file);
    auto addToStream = [&in_stream](QString&& key, QString value) { in_stream << QString("%1 = \"%2\"\n").arg(key, value); };

    {
        addToStream("name", mod.name);
        addToStream("filename", mod.filename);
        addToStream("side", mod.side);

        in_stream << QString("\n[download]\n");
        addToStream("mode", mod.mode);
        addToStream("url", mod.url.toString());
        addToStream("hash-format", mod.hash_format);
        addToStream("hash", mod.hash);

        in_stream << QString("\n[update]\n");
        in_stream << QString("[update.%1]\n").arg(ProviderCaps.name(mod.provider));
        switch (mod.provider) {
            case (ModPlatform::Provider::FLAME):
                in_stream << QString("file-id = %1\n").arg(mod.file_id.toString());
                in_stream << QString("project-id = %1\n").arg(mod.project_id.toString());
                break;
            case (ModPlatform::Provider::MODRINTH):
                addToStream("mod-id", mod.mod_id().toString());
                addToStream("version", mod.version().toString());
                break;
        }
    }

    index_file.flush();
    index_file.close();
}

void V1::deleteModIndex(QDir& index_dir, QString& mod_slug)
{
    auto normalized_fname = indexFileName(mod_slug);
    auto real_fname = getRealIndexName(index_dir, normalized_fname);
    if (real_fname.isEmpty())
        return;

    QFile index_file(index_dir.absoluteFilePath(real_fname));

    if (!index_file.exists()) {
        qWarning() << QString("Tried to delete non-existent mod metadata for %1!").arg(mod_slug);
        return;
    }

    if (!index_file.remove()) {
        qWarning() << QString("Failed to remove metadata for mod %1!").arg(mod_slug);
    }
}

void V1::deleteModIndex(QDir& index_dir, QVariant& mod_id)
{
    for (auto& file_name : index_dir.entryList(QDir::Filter::Files)) {
        auto mod = getIndexForMod(index_dir, file_name);

        if (mod.mod_id() == mod_id) {
            deleteModIndex(index_dir, mod.name);
            break;
        }
    }
}

auto V1::getIndexForMod(QDir& index_dir, QString slug) -> Mod
{
    Mod mod;

    auto normalized_fname = indexFileName(slug);
    auto real_fname = getRealIndexName(index_dir, normalized_fname, true);
    if (real_fname.isEmpty())
        return {};

    toml::table table;
#if TOML_EXCEPTIONS
    try {
        table = toml::parse_file(index_dir.absoluteFilePath(real_fname).toStdString());
    } catch (const toml::parse_error& err) {
        qWarning() << QString("Could not open file %1!").arg(normalized_fname);
        qWarning() << "Reason: " << QString(err.what());
        return {};
    }
#else
    table = toml::parse_file(index_dir.absoluteFilePath(real_fname).toStdString());
    if (!table) {
        qWarning() << QString("Could not open file %1!").arg(normalized_fname);
        qWarning() << "Reason: " << QString(table.error().what());
        return {};
    }
#endif

    // index_file.close();

    mod.slug = slug;

    {  // Basic info
        mod.name = stringEntry(table, "name");
        mod.filename = stringEntry(table, "filename");
        mod.side = stringEntry(table, "side");
    }

    {  // [download] info
        auto download_table = table["download"].as_table();
        if (!download_table) {
            qCritical() << QString("No [download] section found on mod metadata!");
            return {};
        }

        mod.mode = stringEntry(*download_table, "mode");
        mod.url = stringEntry(*download_table, "url");
        mod.hash_format = stringEntry(*download_table, "hash-format");
        mod.hash = stringEntry(*download_table, "hash");
    }

    {  // [update] info
        using Provider = ModPlatform::Provider;

        auto update_table = table["update"];
        if (!update_table || !update_table.is_table()) {
            qCritical() << QString("No [update] section found on mod metadata!");
            return {};
        }

        toml::table* mod_provider_table = nullptr;
        if ((mod_provider_table = update_table[ProviderCaps.name(Provider::FLAME)].as_table())) {
            mod.provider = Provider::FLAME;
            mod.file_id = intEntry(*mod_provider_table, "file-id");
            mod.project_id = intEntry(*mod_provider_table, "project-id");
        } else if ((mod_provider_table = update_table[ProviderCaps.name(Provider::MODRINTH)].as_table())) {
            mod.provider = Provider::MODRINTH;
            mod.mod_id() = stringEntry(*mod_provider_table, "mod-id");
            mod.version() = stringEntry(*mod_provider_table, "version");
        } else {
            qCritical() << QString("No mod provider on mod metadata!");
            return {};
        }
    }

    return mod;
}

auto V1::getIndexForMod(QDir& index_dir, QVariant& mod_id) -> Mod
{
    for (auto& file_name : index_dir.entryList(QDir::Filter::Files)) {
        auto mod = getIndexForMod(index_dir, file_name);

        if (mod.mod_id() == mod_id)
            return mod;
    }

    return {};
}

}  // namespace Packwiz
