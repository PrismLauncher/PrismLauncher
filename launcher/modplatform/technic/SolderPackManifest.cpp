// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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

#include "SolderPackManifest.h"

#include "Json.h"

namespace TechnicSolder {

void loadPack(Pack& v, QJsonObject& obj)
{
    v.recommended = Json::requireString(obj, "recommended");
    v.latest = Json::requireString(obj, "latest");

    auto builds = Json::requireArray(obj, "builds");
    for (const auto buildRaw : builds) {
        auto build = Json::requireString(buildRaw);
        v.builds.append(build);
    }
}

static void loadPackBuildMod(PackBuildMod& b, QJsonObject& obj)
{
    b.name = Json::requireString(obj, "name");
    b.version = Json::ensureString(obj, "version", "");
    b.md5 = Json::requireString(obj, "md5");
    b.url = Json::requireString(obj, "url");
}

void loadPackBuild(PackBuild& v, QJsonObject& obj)
{
    v.minecraft = Json::requireString(obj, "minecraft");

    auto mods = Json::requireArray(obj, "mods");
    for (const auto modRaw : mods) {
        auto modObj = Json::requireObject(modRaw);
        PackBuildMod mod;
        loadPackBuildMod(mod, modObj);
        v.mods.append(mod);
    }
}

}  // namespace TechnicSolder
