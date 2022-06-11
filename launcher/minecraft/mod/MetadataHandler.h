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

#include <memory>

#include "modplatform/packwiz/Packwiz.h"

// launcher/minecraft/mod/Mod.h
class Mod;

/* Abstraction file for easily changing the way metadata is stored / handled
 * Needs to be a class because of -Wunused-function and no C++17 [[maybe_unused]]
 * */
class Metadata {
   public:
    using ModStruct = Packwiz::V1::Mod;

    static auto create(QDir& index_dir, ModPlatform::IndexedPack& mod_pack, ModPlatform::IndexedVersion& mod_version) -> ModStruct
    {
        return Packwiz::V1::createModFormat(index_dir, mod_pack, mod_version);
    }

    static auto create(QDir& index_dir, Mod& internal_mod) -> ModStruct
    {
        return Packwiz::V1::createModFormat(index_dir, internal_mod);
    }

    static void update(QDir& index_dir, ModStruct& mod)
    {
        Packwiz::V1::updateModIndex(index_dir, mod);
    }

    static void remove(QDir& index_dir, QString& mod_name)
    {
        Packwiz::V1::deleteModIndex(index_dir, mod_name);
    }

    static void remove(QDir& index_dir, QVariant& mod_id)
    {
        Packwiz::V1::deleteModIndex(index_dir, mod_id);
    }

    static auto get(QDir& index_dir, QString& mod_name) -> ModStruct
    {
        return Packwiz::V1::getIndexForMod(index_dir, mod_name);
    }

    static auto get(QDir& index_dir, QVariant& mod_id) -> ModStruct
    {
        return Packwiz::V1::getIndexForMod(index_dir, mod_id);
    }
};
