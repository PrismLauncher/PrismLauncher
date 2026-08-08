// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Lenny McLennington <lenny@sneed.church>
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

#pragma once

#include "net/NetRequest.h"

#include <QString>

#include <array>
#include <cstdint>
#include <utility>

namespace PasteUpload {

enum class PasteType : std::uint8_t {
    // 0x0.st
    NullPointer = 0,
    // hastebin.com
    Hastebin = 1,
    // paste.gg
    PasteGG = 2,
    // mclo.gs
    Mclogs = 3,
    // Helpful to get the range of valid values on the enum for input sanitisation:
    First = PasteType::NullPointer,
    Last = PasteType::Mclogs
};
struct PasteTypeInfo {
    QString name;
    QString defaultBase;
    QString endpointPath;
};

inline const std::array<PasteTypeInfo, 4> g_PasteTypes = { { { "0x0.st", "https://0x0.st", "" },
                                                             { "hastebin", "https://hst.sh", "/documents" },
                                                             { "paste.gg", "https://paste.gg", "/api/v1/pastes" },
                                                             { "mclo.gs", "https://api.mclo.gs", "/1/log" } } };

auto make(const QString& log, QString baseUrl, PasteType pasteType) -> std::pair<Net::NetRequest::Ptr, QString*>;

}  // namespace PasteUpload
