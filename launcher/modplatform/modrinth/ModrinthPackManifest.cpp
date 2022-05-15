/* Copyright 2022 kb1000
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ModrinthPackManifest.h"
#include "Json.h"

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

namespace Modrinth {

void loadIndexedPack(Modpack& pack, QJsonObject& obj)
{
    pack.id = Json::ensureString(obj, "project_id");

    pack.name = Json::ensureString(obj, "title");
    pack.description = Json::ensureString(obj, "description");
    pack.authors << Json::ensureString(obj, "author");
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

    for (auto file_iter : files) {
        File indexed_file;
        auto parent = Json::requireObject(file_iter);
        if (!Json::ensureBoolean(parent, "primary", false)) {
            continue;
        }

        file.download_url = Json::requireString(parent, "url");
        break;
    }

    if(file.download_url.isEmpty())
        return {};

    return file;
}        

}  // namespace Modrinth
