// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
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

#include "ATLShareCode.h"

#include "Json.h"

namespace ATLauncher {

static void loadShareCodeMod(ShareCodeMod& m, QJsonObject& obj)
{
    m.selected = Json::requireBoolean(obj, "selected");
    m.name = Json::requireString(obj, "name");
}

static void loadShareCode(ShareCode& c, QJsonObject& obj)
{
    c.pack = Json::requireString(obj, "pack");
    c.version = Json::requireString(obj, "version");

    auto mods = Json::requireObject(obj, "mods");
    auto optional = Json::requireArray(mods, "optional");
    for (const auto modRaw : optional) {
        auto modObj = Json::requireObject(modRaw);
        ShareCodeMod mod;
        loadShareCodeMod(mod, modObj);
        c.mods.append(mod);
    }
}

void loadShareCodeResponse(ShareCodeResponse& r, QJsonObject& obj)
{
    r.error = Json::requireBoolean(obj, "error");
    r.code = Json::requireInteger(obj, "code");

    if (obj.contains("message") && !obj.value("message").isNull())
        r.message = Json::requireString(obj, "message");

    if (!r.error) {
        auto dataRaw = Json::requireObject(obj, "data");
        loadShareCode(r.data, dataRaw);
    }
}

}
