// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Prism Launcher Contributors
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

#include <QString>

class IconList;

/**
 * Icon contract. Domain code saves a launch icon. UI code uses list() for the model
 * Application already owns. A null list means icons are unavailable.
 */
class IconStore {
   public:
    virtual ~IconStore() = default;

    virtual void saveIcon(const QString& key, const QString& path, const char* format) const = 0;
    virtual IconList* list() const = 0;
};
