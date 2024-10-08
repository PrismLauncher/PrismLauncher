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

#pragma once

#include <memory>

#include "modplatform/packwiz/Packwiz.h"

// launcher/minecraft/mod/Mod.h
class Mod;

namespace Metadata {
using ModStruct = Packwiz::V1::Mod;
using ModSide = Packwiz::V1::Side;

inline auto create(const QDir& index_dir, ModPlatform::IndexedPack& mod_pack, ModPlatform::IndexedVersion& mod_version) -> ModStruct
{
    return Packwiz::V1::createModFormat(index_dir, mod_pack, mod_version);
}

inline auto create(const QDir& index_dir, Mod& internal_mod, QString mod_slug) -> ModStruct
{
    return Packwiz::V1::createModFormat(index_dir, internal_mod, std::move(mod_slug));
}

inline void update(const QDir& index_dir, ModStruct& mod)
{
    Packwiz::V1::updateModIndex(index_dir, mod);
}

inline void remove(const QDir& index_dir, QString mod_slug)
{
    Packwiz::V1::deleteModIndex(index_dir, mod_slug);
}

inline void remove(const QDir& index_dir, QVariant& mod_id)
{
    Packwiz::V1::deleteModIndex(index_dir, mod_id);
}

inline auto get(const QDir& index_dir, QString mod_slug) -> ModStruct
{
    return Packwiz::V1::getIndexForMod(index_dir, std::move(mod_slug));
}

inline auto get(const QDir& index_dir, QVariant& mod_id) -> ModStruct
{
    return Packwiz::V1::getIndexForMod(index_dir, mod_id);
}

inline auto modSideToString(ModSide side) -> QString
{
    return Packwiz::V1::sideToString(side);
}

};  // namespace Metadata
