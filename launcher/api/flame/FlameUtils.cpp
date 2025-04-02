// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only AND Apache-2.0
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2025 Trial97 <alexandru.tripon97@gmail.com>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "FlameUtils.h"
#include "Version.h"
#include "modplatform/flame/FlameModIndex.h"

namespace FlameUtils {
int getClassId(Platform::ResourceType type)
{
    switch (type) {
        default:
        case Platform::ResourceType::Mod:
            return 6;
        case Platform::ResourceType::ResourcePack:
            return 12;
        case Platform::ResourceType::ShaderPack:
            return 6552;
        case Platform::ResourceType::Modpack:
            return 4471;
    }
}
Platform::ResourceType getResourceType(int classId)
{
    switch (classId) {
        case 17:  // Worlds
            return Platform::ResourceType::World;
        case 6:  // Mods
            return Platform::ResourceType::Mod;
        case 12:  // Resource Packs
                  // return Platform::ResourceType::ResourcePack; // not really a resourcepack
            /* fallthrough */
        case 4546:  // Customization
                    // return Platform::ResourceType::ShaderPack; // not really a shaderPack
            /* fallthrough */
        case 4471:  // Modpacks
            /* fallthrough */
        case 5:  // Bukkit Plugins
            /* fallthrough */
        case 4559:  // Addons
            /* fallthrough */
        default:
            return Platform::ResourceType::Unknown;
    }
}
int getMappedModLoader(Platform::ModLoader loaders)
{
    // https://docs.curseforge.com/rest-api/#tocS_ModLoaderType
    switch (loaders) {
        case Platform::ModLoader::Forge:
            return 1;
        case Platform::ModLoader::Cauldron:
            return 2;
        case Platform::ModLoader::LiteLoader:
            return 3;
        case Platform::ModLoader::Fabric:
            return 4;
        case Platform::ModLoader::Quilt:
            return 5;
        case Platform::ModLoader::NeoForge:
            return 6;
    }
    return 0;
}
QStringList getModLoaderStrings(Platform::ModLoaders types)
{
    QStringList l;
    for (auto loader :
         { Platform::ModLoader::NeoForge, Platform::ModLoader::Forge, Platform::ModLoader::Fabric, Platform::ModLoader::Quilt }) {
        if (types & loader) {
            l << QString::number(getMappedModLoader(loader));
        }
    }
    return l;
}
QString getModLoaderFilters(Platform::ModLoaders types)
{
    return "[" + getModLoaderStrings(types).join(',') + "]";
}
Platform::Version loadIndexedPackVersion(QJsonObject& obj, Platform::ResourceType resourceType)
{
    auto arr = FlameMod::loadIndexedPackVersion(obj);
    if (resourceType != Platform::ResourceType::TexturePack) {
        return arr;
    }
    // FIXME: Client-side version filtering. This won't take into account any user-selected filtering.
    auto const& mc_versions = arr.mcVersion;

    if (std::any_of(mc_versions.constBegin(), mc_versions.constEnd(),
                    [](auto const& mc_version) { return Version(mc_version) <= Version("1.6"); })) {
        return arr;
    }
    return {};
}
}  // namespace FlameUtils