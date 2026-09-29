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

#include "EnumWrapper.h"
#include "net/Request.h"

#include <QString>

#include <array>
#include <cstdint>
#include <utility>

namespace PasteUpload {

enum class TypeValue : std::uint8_t {
    // 0x0.st
    NullPointer = 0,
    // hastebin.com
    Hastebin = 1,
    // paste.gg
    PasteGG = 2,
    // mclo.gs
    Mclogs = 3,
    Invalid = 4
};

struct Type : EnumWrapper<Type, TypeValue> {
    static constexpr auto invalid() { return Invalid; };
    static constexpr auto mapping()
    {
        return std::array{ std::pair{ NullPointer, "0x0.st" }, std::pair{ Hastebin, "hastebin" }, std::pair{ PasteGG, "paste.gg" },
                           std::pair{ Mclogs, "mclo.gs" } };
    };

    explicit Type(int v) : Type{ v >= 0 && v < static_cast<int>(Invalid) ? static_cast<TypeValue>(v) : Invalid } {}
    int toInt() const { return std::to_underlying(value()); }

    QString defaultBase() const;
    QString endpointPath() const;
    std::pair<Net::Request::Ptr, QString*> make(QString log, QString baseUrl) const;

    using enum TypeValue;
    using Base = EnumWrapper<Type, TypeValue>;
    using Base::Base; /* inherit ctor */
};

}  // namespace PasteUpload
