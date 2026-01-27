// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 Prism Launcher Contributors
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

#include "TechnicPackManifest.h"

#include "Json.h"

namespace TechnicPlatform {

void loadPackInfo(PackInfo& info, const QJsonObject& obj)
{
    info.name = Json::requireString(obj, "name");
    info.displayName = obj.value("displayName").toString(info.name);
    info.platformUrl = obj.value("platformUrl").toString();
    info.minecraft = obj.value("minecraft").toString();
    info.version = obj.value("version").toString();
    info.description = obj.value("description").toString();
    info.author = obj.value("user").toString();

    // Check for Solder URL
    QString solderUrl = obj.value("solder").toString();
    if (!solderUrl.isEmpty()) {
        info.isSolder = true;
        info.solderUrl = solderUrl;
    } else {
        // Non-Solder pack: get direct download URL
        info.url = obj.value("url").toString();
    }

    info.isLoaded = true;
}

void loadSolderPackInfo(SolderPackInfo& info, const QJsonObject& obj)
{
    info.name = Json::requireString(obj, "name");
    info.displayName = obj.value("display_name").toString(info.name);
    info.recommended = obj.value("recommended").toString();
    info.latest = obj.value("latest").toString();

    auto buildsArray = Json::requireArray(obj, "builds");
    for (const auto& buildVal : buildsArray) {
        info.builds.append(Json::requireString(buildVal));
    }

    info.isLoaded = true;
}

static void loadSolderMod(SolderMod& mod, const QJsonObject& obj)
{
    mod.name = Json::requireString(obj, "name");
    mod.version = obj.value("version").toString();
    mod.md5 = Json::requireString(obj, "md5");
    mod.url = Json::requireString(obj, "url");
}

void loadSolderBuild(SolderBuild& build, const QJsonObject& obj)
{
    build.minecraft = Json::requireString(obj, "minecraft");

    auto modsArray = Json::requireArray(obj, "mods");
    for (const auto& modVal : modsArray) {
        auto modObj = Json::requireObject(modVal);
        SolderMod mod;
        loadSolderMod(mod, modObj);
        build.mods.append(mod);
    }
}

}  // namespace TechnicPlatform
