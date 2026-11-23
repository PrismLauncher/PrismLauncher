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

#include "icons/IconList.h"
#include "services/IconStore.h"

/** Non-owning IconStore over the IconList Application already owns. */
class IconListIconStore : public IconStore {
   public:
    explicit IconListIconStore(IconList* icons) : m_icons(icons) {}

    void saveIcon(const QString& key, const QString& path, const char* format) const override
    {
        if (m_icons != nullptr) {
            m_icons->saveIcon(key, path, format);
        }
    }

    IconList* list() const override { return m_icons; }

   private:
    IconList* m_icons;
};
