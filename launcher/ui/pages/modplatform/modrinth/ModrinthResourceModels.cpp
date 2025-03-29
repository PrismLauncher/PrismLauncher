// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include "ModrinthResourceModels.h"

#include "modplatform/modrinth/ModrinthAPI.h"
#include "modplatform/modrinth/ModrinthPackIndex.h"

namespace ResourceDownload {

ModrinthModModel::ModrinthModModel(BaseInstance& base) : ModModel(base, new ModrinthAPI) {}

auto ModrinthModModel::loadDependencyVersions(const ModPlatform::Dependency& m, QJsonArray& arr) -> ModPlatform::IndexedVersion
{
    return ::Modrinth::loadDependencyVersions(m, arr, &m_base_instance);
}

}  // namespace ResourceDownload
