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

#include "TexturePack.h"

#include <QDebug>
#include <QMap>
#include <QRegularExpression>

#include "MTPixmapCache.h"

#include "minecraft/mod/tasks/LocalTexturePackParseTask.h"

void TexturePack::setDescription(QString new_description)
{
    QMutexLocker locker(&m_data_lock);

    m_description = new_description;
}

void TexturePack::setImage(QImage new_image) const
{
    QMutexLocker locker(&m_data_lock);

    Q_ASSERT(!new_image.isNull());

    if (m_pack_image_cache_key.key.isValid())
        PixmapCache::remove(m_pack_image_cache_key.key);

    // scale the image to avoid flooding the pixmapcache
    auto pixmap = QPixmap::fromImage(new_image.scaled({64, 64}, Qt::AspectRatioMode::KeepAspectRatioByExpanding));

    m_pack_image_cache_key.key = PixmapCache::insert(pixmap);
    m_pack_image_cache_key.was_ever_used = true;
}

QPixmap TexturePack::image(QSize size, Qt::AspectRatioMode mode) const
{
    QPixmap cached_image;
    if (PixmapCache::find(m_pack_image_cache_key.key, &cached_image)) {
        if (size.isNull())
            return cached_image;
        return cached_image.scaled(size, mode);
    }

    // No valid image we can get
    if (!m_pack_image_cache_key.was_ever_used) {
        return {};
    } else {
        qDebug() << "Texture Pack" << name() << "Had it's image evicted from the cache. reloading...";
        PixmapCache::markCacheMissByEviciton();
    }

    // Imaged got evicted from the cache. Re-process it and retry.
    TexturePackUtils::processPackPNG(*this);
    return image(size);
}

bool TexturePack::valid() const
{
    return m_description != nullptr;
}
