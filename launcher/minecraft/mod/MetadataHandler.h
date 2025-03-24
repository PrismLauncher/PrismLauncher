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

#include "modplatform/packwiz/Packwiz.h"

namespace Metadata {
using ModStruct = Packwiz::V1::Mod;

inline ModStruct create(const QDir& index_dir, ModPlatform::IndexedPack& mod_pack, ModPlatform::IndexedVersion& mod_version)
{
    return Packwiz::V1::createModFormat(index_dir, mod_pack, mod_version);
}

inline void update(const QDir& index_dir, ModStruct& mod)
{
    Packwiz::V1::updateModIndex(index_dir, mod);
}

inline void remove(const QDir& index_dir, QString mod_slug)
{
    Packwiz::V1::deleteModIndex(index_dir, mod_slug);
}

inline ModStruct get(const QDir& index_dir, QString mod_slug)
{
    return Packwiz::V1::getIndexForMod(index_dir, std::move(mod_slug));
}

inline ModStruct get(const QDir& index_dir, QVariant& mod_id)
{
    return Packwiz::V1::getIndexForMod(index_dir, mod_id);
}

};  // namespace Metadata
