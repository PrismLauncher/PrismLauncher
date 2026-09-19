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

Result<> loadPack(Pack& v, const QJsonObject& obj)
{
    TRY_INTO(v.recommended, Json::requireString(obj, "recommended"))
    TRY_INTO(v.latest, Json::requireString(obj, "latest"))

    TRY_INTO(const auto& builds, Json::requireArray(obj, "builds"))
    for (const auto buildRaw : builds) {
        TRY_INTO(const auto& build, Json::requireString(buildRaw))
        v.builds.append(build);
    }
    return {};
}

static Result<> loadPackBuildMod(PackBuildMod& b, const QJsonObject& obj)
{
    TRY_INTO(b.name, Json::requireString(obj, "name"))
    b.version = obj["version"].toString("");
    TRY_INTO(b.md5, Json::requireString(obj, "md5"))
    TRY_INTO(b.url, Json::requireString(obj, "url"))
    return {};
}

Result<> loadPackBuild(PackBuild& v, const QJsonObject& obj)
{
    TRY_INTO(v.minecraft, Json::requireString(obj, "minecraft"))

    TRY_INTO(const auto& mods, Json::requireArray(obj, "mods"))
    for (const auto modRaw : mods) {
        PackBuildMod mod;
        TRY(Json::requireObject(modRaw).and_then([&mod](const auto& v) { return loadPackBuildMod(mod, v); }))
        v.mods.append(mod);
    }
    return {};
}

}  // namespace TechnicSolder
