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

enum Formats { HTML, MARKDOWN, PLAINTXT, JSON, CSV, CUSTOM };
enum OptionalData { Authors = 1 << 0, Url = 1 << 1, Version = 1 << 2, FileName = 1 << 3 };
QString exportToModList(QList<Mod*> mods, Formats format, OptionalData extraData);
QString exportToModList(QList<Mod*> mods, QString lineTemplate);
}  // namespace ExportToModList
