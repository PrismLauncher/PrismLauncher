// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include "Resource.h"

#include <QImage>
#include <QMutex>
#include <QPixmap>
#include <QPixmapCache>

class Version;

class TexturePack : public Resource {
   public:
    TexturePack(QFileInfo file_info) : Resource(file_info)
    {
        connect(&m_iconCache, &ResourceIconCache::iconLoaded, this, &Resource::iconChanged);
    }

    /** Gets the description of the texture pack. */
    QString description() const { return m_description; }

    /** Thread-safe. */
    void setDescription(QString new_description);

    void loadIcon() override;

    QPixmap icon(QSize size) const override { return m_iconCache.icon(size); }

    QIcon fallbackIcon() const override { return QIcon::fromTheme("datapacks"); }

    bool valid() const override;

   protected:
    mutable QMutex m_data_lock;

    /** The texture pack's description, as defined in the pack.txt file.
     */
    QString m_description;
    ResourceIconCache m_iconCache;
};
