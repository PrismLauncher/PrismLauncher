// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
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

#include "ui/pages/modplatform/modrinth/ModrinthResourcePages.h"

#include "modplatform/modrinth/ModrinthAPI.h"
#include "modplatform/modrinth/ModrinthPackIndex.h"

namespace ResourceDownload {

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
const char* ModrinthModModel::sorts[5]{ "relevance", "downloads", "follows", "updated", "newest" };

ModrinthModModel::ModrinthModModel(ModrinthModPage* parent) : ModModel(parent, new ModrinthAPI){};

void ModrinthModModel::loadIndexedPack(ModPlatform::IndexedPack& m, QJsonObject& obj)
{
    ::Modrinth::loadIndexedPack(m, obj);
}

void ModrinthModModel::loadExtraPackInfo(ModPlatform::IndexedPack& m, QJsonObject& obj)
{
    ::Modrinth::loadExtraPackData(m, obj);
}

void ModrinthModModel::loadIndexedPackVersions(ModPlatform::IndexedPack& m, QJsonArray& arr)
{
    ::Modrinth::loadIndexedPackVersions(m, arr, APPLICATION->network(), &m_associated_page->m_base_instance);
}

auto ModrinthModModel::documentToArray(QJsonDocument& obj) const -> QJsonArray
{
    return obj.object().value("hits").toArray();
}

}  // namespace ResourceDownload
