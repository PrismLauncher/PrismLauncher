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

#include "ATLShareCode.h"

#include "Json.h"

namespace ATLauncher {

Result<> loadShareCodeMod(ShareCodeMod& m, QJsonObject& obj)
{
    TRY_INTO(m.selected, Json::requireBoolean(obj, "selected"))
    TRY_INTO(m.name, Json::requireString(obj, "name"))
    return {};
}

Result<> loadShareCode(ShareCode& c, QJsonObject& obj)
{
    TRY_INTO(c.pack, Json::requireString(obj, "pack"))
    TRY_INTO(c.version, Json::requireString(obj, "version"))

    auto mods = Json::requireObject(obj, "mods");
    TRY(mods)
    auto optional = Json::requireArray(mods.value(), "optional");
    TRY(optional)
    for (const auto modRaw : optional.value()) {
        auto modObj = Json::requireObject(modRaw);
        TRY(modObj)
        ShareCodeMod mod;
        TRY(loadShareCodeMod(mod, modObj.value()))
        c.mods.append(mod);
    }
    return {};
}

Result<> loadShareCodeResponse(ShareCodeResponse& r, QJsonObject& obj)
{
    TRY_INTO(r.error, Json::requireBoolean(obj, "error"))
    TRY_INTO(r.code, Json::requireInteger(obj, "code"))

    if (obj.contains("message") && !obj.value("message").isNull()) {
        TRY_INTO(r.message, Json::requireString(obj, "message"))
    }

    if (!r.error) {
        auto dataRaw = Json::requireObject(obj, "data");
        TRY(dataRaw)
        TRY(loadShareCode(r.data, dataRaw.value()))
    }
    return {};
}

}  // namespace ATLauncher
