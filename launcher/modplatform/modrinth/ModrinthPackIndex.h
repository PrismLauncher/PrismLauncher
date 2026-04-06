// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

#include "BaseInstance.h"

namespace Modrinth {

void loadIndexedPack(ModPlatform::IndexedPack& pack, QJsonObject& obj);
void loadExtraPackData(ModPlatform::IndexedPack& pack, QJsonObject& obj);
auto loadIndexedPackVersion(QJsonObject& obj, const QString& preferred_hash_type = "sha512", const QString& preferred_file_name = "")
    -> ModPlatform::IndexedVersion;

}  // namespace Modrinth
