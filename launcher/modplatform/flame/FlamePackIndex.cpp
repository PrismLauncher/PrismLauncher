// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2026 Trial97 <alexandru.tripon97@gmail.com>
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

#include "FlamePackIndex.h"

#include "Json.h"
#include "modplatform/ModIndex.h"
#include "modplatform/flame/FlameAPI.h"

namespace Flame::Parse {
Result<> loadIndexedPack(ModPlatform::IndexedPack& pack, const QJsonObject& obj)
{
    TRY_INTO(pack.addonId, Json::requireInteger(obj, "id"))
    pack.provider = ModPlatform::ResourceProvider::FLAME;
    TRY_INTO(pack.name, Json::requireString(obj, "name"))
    TRY_INTO(pack.slug, Json::requireString(obj, "slug"))
    pack.websiteUrl = obj["links"].toObject()["websiteUrl"].toString("");
    pack.description = obj["summary"].toString("");

    QJsonObject logo = obj["logo"].toObject();
    pack.logoName = logo["title"].toString();
    pack.logoUrl = logo["thumbnailUrl"].toString();
    if (pack.logoUrl.isEmpty()) {
        pack.logoUrl = logo["url"].toString();
    }

    auto authors = obj["authors"].toArray();
    if (!authors.isEmpty()) {
        pack.authors.clear();
        for (auto authorIter : authors) {
            TRY_INTO(const auto& author, Json::requireObject(authorIter))
            ModPlatform::ModpackAuthor packAuthor;
            TRY_INTO(packAuthor.name, Json::requireString(author, "name"))
            TRY_INTO(packAuthor.url, Json::requireString(author, "url"))
            pack.authors.append(packAuthor);
        }
    }

    pack.resourceType = FlameAPI::getResourceType(obj["classId"].toInt(0));
    pack.extraDataLoaded = false;

    auto linksObj = obj["links"].toObject();

    pack.extraData.issuesUrl = linksObj["issuesUrl"].toString();
    if (pack.extraData.issuesUrl.endsWith('/')) {
        pack.extraData.issuesUrl.chop(1);
    }

    pack.extraData.sourceUrl = linksObj["sourceUrl"].toString();
    if (pack.extraData.sourceUrl.endsWith('/')) {
        pack.extraData.sourceUrl.chop(1);
    }

    pack.extraData.wikiUrl = linksObj["wikiUrl"].toString();
    if (pack.extraData.wikiUrl.endsWith('/')) {
        pack.extraData.wikiUrl.chop(1);
    }

    if (!pack.extraData.body.isEmpty()) {
        pack.extraDataLoaded = true;
    }
    return {};
}

Result<QList<ModPlatform::IndexedPack>> parseProjectList(const QByteArray& response)
{
    QList<ModPlatform::IndexedPack> newList;
    TRY_INTO(auto doc, Json::requireDocument(response, "ResourceAPI")
                           .and_then([](const auto& v) { return Json::requireObject(v); })
                           .and_then([](const auto& v) { return Json::requireArray(v, "data"); }))

    for (auto packRaw : doc) {
        auto packObj = packRaw.toObject();

        ModPlatform::IndexedPack pack;
        TRY(Flame::Parse::loadIndexedPack(pack, packObj))
        newList << pack;
    }
    return newList;
}

}  // namespace Flame::Parse
