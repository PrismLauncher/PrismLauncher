// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (c) 2023-2025 Trial97 <alexandru.tripon97@gmail.com>
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

#pragma once

#include <QMetaType>

namespace Platform {
enum ModLoader { NeoForge = 1 << 0, Forge = 1 << 1, Cauldron = 1 << 2, LiteLoader = 1 << 3, Fabric = 1 << 4, Quilt = 1 << 5 };
Q_DECLARE_FLAGS(ModLoaders, ModLoader)

namespace ModloaderUtils {
QList<ModLoader> toList(ModLoaders flags) noexcept;
QString toString(ModLoader type) noexcept;
ModLoader fromString(QString type) noexcept;

constexpr bool hasSingleSelected(ModLoaders l) noexcept
{
    auto x = static_cast<int>(l);
    return x && !(x & (x - 1));
}
}  // namespace ModloaderUtils
}  // namespace Platform