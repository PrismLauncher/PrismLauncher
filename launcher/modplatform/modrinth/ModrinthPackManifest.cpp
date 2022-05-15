// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
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
 *      Copyright 2022 kb1000
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

#include "ModrinthPackManifest.h"
#include "Json.h"

#include "modplatform/modrinth/ModrinthAPI.h"

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

static ModrinthAPI api;

namespace Modrinth {

void loadIndexedPack(Modpack& pack, QJsonObject& obj)
{
    pack.id = Json::ensureString(obj, "project_id");

    pack.name = Json::ensureString(obj, "title");
    pack.description = Json::ensureString(obj, "description");
    auto temp_author_name = Json::ensureString(obj, "author");
    pack.author = std::make_tuple(temp_author_name, api.getAuthorURL(temp_author_name));
    pack.iconName = QString("modrinth_%1").arg(Json::ensureString(obj, "slug"));
    pack.iconUrl = Json::ensureString(obj, "icon_url");
}

void loadIndexedInfo(Modpack& pack, QJsonObject& obj)
{
    pack.extra.body = Json::ensureString(obj, "body");
    pack.extra.projectUrl = QString("https://modrinth.com/modpack/%1").arg(Json::ensureString(obj, "slug"));
    pack.extra.sourceUrl = Json::ensureString(obj, "source_url");
    pack.extra.wikiUrl = Json::ensureString(obj, "wiki_url");

    pack.extraInfoLoaded = true;
}

void loadIndexedVersions(Modpack& pack, QJsonDocument& doc)
{
    QVector<ModpackVersion> unsortedVersions;

    auto arr = Json::requireArray(doc);

    for (auto versionIter : arr) {
        auto obj = Json::requireObject(versionIter);
        auto file = loadIndexedVersion(obj);

        if(!file.id.isEmpty()) // Heuristic to check if the returned value is valid
            unsortedVersions.append(file);
    }
    auto orderSortPredicate = [](const ModpackVersion& a, const ModpackVersion& b) -> bool {
        // dates are in RFC 3339 format
        return a.date > b.date;
    };

    std::sort(unsortedVersions.begin(), unsortedVersions.end(), orderSortPredicate);

    pack.versions.swap(unsortedVersions);

    pack.versionsLoaded = true;
}

auto loadIndexedVersion(QJsonObject &obj) -> ModpackVersion
{
    ModpackVersion file;

    file.name = Json::requireString(obj, "name");
    file.version = Json::requireString(obj, "version_number");

    file.id = Json::requireString(obj, "id");
    file.project_id = Json::requireString(obj, "project_id");
    
    file.date = Json::requireString(obj, "date_published");

    auto files = Json::requireArray(obj, "files");

    qWarning() << files;

    for (auto file_iter : files) {
        File indexed_file;
        auto parent = Json::requireObject(file_iter);
        auto is_primary = Json::ensureBoolean(parent, "primary", false);
        if (!is_primary) {
            auto filename = Json::ensureString(parent, "filename");
            // Checking suffix here is fine because it's the response from Modrinth,
            // so one would assume it will always be in English.
            if(!filename.endsWith("mrpack") && !filename.endsWith("zip"))
                continue;
        }

        file.download_url = Json::requireString(parent, "url");
        if(is_primary)
            break;
    }

    if(file.download_url.isEmpty())
        return {};

    return file;
}        

}  // namespace Modrinth
