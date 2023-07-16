// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
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
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<Resource>;

    TexturePack(QObject* parent = nullptr) : Resource(parent) {}
    TexturePack(QFileInfo file_info) : Resource(file_info) {}

    /** Gets the description of the texture pack. */
    [[nodiscard]] QString description() const { return m_description; }

    /** Gets the image of the texture pack, converted to a QPixmap for drawing, and scaled to size. */
    [[nodiscard]] QPixmap image(QSize size, Qt::AspectRatioMode mode = Qt::AspectRatioMode::IgnoreAspectRatio) const;

    /** Thread-safe. */
    void setDescription(QString new_description);

    /** Thread-safe. */
    void setImage(QImage new_image) const;

    bool valid() const override;

   protected:
    mutable QMutex m_data_lock;

    /** The texture pack's description, as defined in the pack.txt file.
     */
    QString m_description;

    /** The texture pack's image file cache key, for access in the QPixmapCache global instance.
     *
     *  The 'was_ever_used' state simply identifies whether the key was never inserted on the cache (true),
     *  so as to tell whether a cache entry is inexistent or if it was just evicted from the cache.
     */
    struct {
        QPixmapCache::Key key;
        bool was_ever_used = false;
    } mutable m_pack_image_cache_key;
};
