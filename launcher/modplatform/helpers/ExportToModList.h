// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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
#include <QList>
#include <QString>
#include "minecraft/mod/Mod.h"

namespace ExportToModList {

enum Formats : std::uint8_t { HTML, MARKDOWN, PLAINTXT, JSON, CSV, CUSTOM };
enum OptionalDataValue : std::uint8_t { None = 0, Authors = 1U << 0U, Url = 1U << 1U, Version = 1U << 2U, FileName = 1U << 3U };
Q_DECLARE_FLAGS(OptionalData, OptionalDataValue)

QString exportToModList(const QList<Mod*>& mods, Formats format, OptionalData extraData);
QString exportToModList(const QList<Mod*>& mods, const QString& lineTemplate);
}  // namespace ExportToModList
