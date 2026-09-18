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

Result<> loadShareCodeMod(ShareCodeMod& m, const QJsonObject& obj)
{
    TRY_INTO(m.selected, Json::requireBoolean(obj, "selected"))
    TRY_INTO(m.name, Json::requireString(obj, "name"))
    return {};
}

Result<> loadShareCode(ShareCode& c, const QJsonObject& obj)
{
    TRY_INTO(c.pack, Json::requireString(obj, "pack"))
    TRY_INTO(c.version, Json::requireString(obj, "version"))

    TRY_INTO(const auto& optional,
             Json::requireObject(obj, "mods").and_then([](const auto& v) { return Json::requireArray(v, "optional"); }))
    for (const auto modRaw : optional) {
        ShareCodeMod mod;
        TRY(Json::requireObject(modRaw).and_then([&mod](const auto& v) { return loadShareCodeMod(mod, v); }))
        c.mods.append(mod);
    }
    return {};
}

Result<> loadShareCodeResponse(ShareCodeResponse& r, const QJsonObject& obj)
{
    TRY_INTO(r.error, Json::requireBoolean(obj, "error"))
    TRY_INTO(r.code, Json::requireInteger(obj, "code"))

    if (obj.contains("message") && !obj.value("message").isNull()) {
        TRY_INTO(r.message, Json::requireString(obj, "message"))
    }

    if (!r.error) {
        TRY(Json::requireObject(obj, "data").and_then([&r](const auto& v) { return loadShareCode(r.data, v); }))
    }
    return {};
}

}  // namespace ATLauncher
