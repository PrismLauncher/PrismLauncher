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

#pragma once

#include "modplatform/ModIndex.h"

#include <QString>
#include <QUrl>
#include <QVariant>

struct toml_table_t;
class QDir;

// Mod from launcher/minecraft/mod/Mod.h
class Mod;

namespace Packwiz {

auto getRealIndexName(QDir& index_dir, QString normalized_index_name, bool should_match = false) -> QString;

auto stringEntry(toml_table_t* parent, const char* entry_name) -> QString;
auto intEntry(toml_table_t* parent, const char* entry_name) -> int;

class V1 {
   public:
    struct Mod {
        QString name {};
        QString filename {};
        // FIXME: make side an enum
        QString side {"both"};

        // [download]
        QString mode {};
        QUrl url {};
        QString hash_format {};
        QString hash {};

        // [update]
        ModPlatform::Provider provider {};
        QVariant file_id {};
        QVariant project_id {};

       public:
        // This is a totally heuristic, but should work for now.
        auto isValid() const -> bool { return !name.isEmpty() && !project_id.isNull(); }

        // Different providers can use different names for the same thing
        // Modrinth-specific
        auto mod_id() -> QVariant& { return project_id; }
        auto version() -> QVariant& { return file_id; }
    };

    /* Generates the object representing the information in a mod.pw.toml file via
     * its common representation in the launcher, when downloading mods.
     * */
    static auto createModFormat(QDir& index_dir, ModPlatform::IndexedPack& mod_pack, ModPlatform::IndexedVersion& mod_version) -> Mod;
    /* Generates the object representing the information in a mod.pw.toml file via
     * its common representation in the launcher.
     * */
    static auto createModFormat(QDir& index_dir, ::Mod& internal_mod) -> Mod;

    /* Updates the mod index for the provided mod.
     * This creates a new index if one does not exist already
     * TODO: Ask the user if they want to override, and delete the old mod's files, or keep the old one.
     * */
    static void updateModIndex(QDir& index_dir, Mod& mod);

    /* Deletes the metadata for the mod with the given name. If the metadata doesn't exist, it does nothing. */
    static void deleteModIndex(QDir& index_dir, QString& mod_name);

    /* Gets the metadata for a mod with a particular name.
     * If the mod doesn't have a metadata, it simply returns an empty Mod object.
     * */
    static auto getIndexForMod(QDir& index_dir, QString& index_file_name) -> Mod;
};

} // namespace Packwiz
