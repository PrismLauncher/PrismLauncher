// SPDX-FileCopyrightText: 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
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

#include "DataPack.h"

#include <QDebug>
#include <QMap>
#include <QRegularExpression>

#include "MTPixmapCache.h"
#include "Version.h"
#include "minecraft/mod/tasks/LocalDataPackParseTask.h"

// Values taken from:
// https://minecraft.wiki/w/Tutorials/Creating_a_data_pack#%22pack_format%22
static const QMap<int, std::pair<Version, Version>> s_pack_format_versions = {
    { 4, { Version("1.13"), Version("1.14.4") } },    { 5, { Version("1.15"), Version("1.16.1") } },
    { 6, { Version("1.16.2"), Version("1.16.5") } },  { 7, { Version("1.17"), Version("1.17.1") } },
    { 8, { Version("1.18"), Version("1.18.1") } },    { 9, { Version("1.18.2"), Version("1.18.2") } },
    { 10, { Version("1.19"), Version("1.19.3") } },   { 11, { Version("23w03a"), Version("23w05a") } },
    { 12, { Version("1.19.4"), Version("1.19.4") } }, { 13, { Version("23w12a"), Version("23w14a") } },
    { 14, { Version("23w16a"), Version("23w17a") } }, { 15, { Version("1.20"), Version("1.20") } },
};

void DataPack::setPackFormat(int new_format_id)
{
    QMutexLocker locker(&m_data_lock);

    if (!s_pack_format_versions.contains(new_format_id)) {
        qWarning() << "Pack format '" << new_format_id << "' is not a recognized data pack id!";
    }

    m_pack_format = new_format_id;
}

void DataPack::setDescription(QString new_description)
{
    QMutexLocker locker(&m_data_lock);

    m_description = new_description;
}

void DataPack::setImage(QImage new_image) const
{
    QMutexLocker locker(&m_data_lock);

    Q_ASSERT(!new_image.isNull());

    if (m_pack_image_cache_key.key.isValid())
        PixmapCache::instance().remove(m_pack_image_cache_key.key);

    // scale the image to avoid flooding the pixmapcache
    auto pixmap =
        QPixmap::fromImage(new_image.scaled({ 64, 64 }, Qt::AspectRatioMode::KeepAspectRatioByExpanding, Qt::SmoothTransformation));

    m_pack_image_cache_key.key = PixmapCache::instance().insert(pixmap);
    m_pack_image_cache_key.was_ever_used = true;

    // This can happen if the pixmap is too big to fit in the cache :c
    if (!m_pack_image_cache_key.key.isValid()) {
        qWarning() << "Could not insert a image cache entry! Ignoring it.";
        m_pack_image_cache_key.was_ever_used = false;
    }
}

QPixmap DataPack::image(QSize size, Qt::AspectRatioMode mode) const
{
    QPixmap cached_image;
    if (PixmapCache::instance().find(m_pack_image_cache_key.key, &cached_image)) {
        if (size.isNull())
            return cached_image;
        return cached_image.scaled(size, mode, Qt::SmoothTransformation);
    }

    // No valid image we can get
    if (!m_pack_image_cache_key.was_ever_used) {
        return {};
    } else {
        qDebug() << "Resource Pack" << name() << "Had it's image evicted from the cache. reloading...";
        PixmapCache::markCacheMissByEviciton();
    }

    // Imaged got evicted from the cache. Re-process it and retry.
    DataPackUtils::processPackPNG(*this);
    return image(size);
}

std::pair<Version, Version> DataPack::compatibleVersions() const
{
    if (!s_pack_format_versions.contains(m_pack_format)) {
        return { {}, {} };
    }

    return s_pack_format_versions.constFind(m_pack_format).value();
}

std::pair<int, bool> DataPack::compare(const Resource& other, SortType type) const
{
    auto const& cast_other = static_cast<DataPack const&>(other);

    switch (type) {
        default: {
            auto res = Resource::compare(other, type);
            if (res.first != 0)
                return res;
            break;
        }
        case SortType::PACK_FORMAT: {
            auto this_ver = packFormat();
            auto other_ver = cast_other.packFormat();

            if (this_ver > other_ver)
                return { 1, type == SortType::PACK_FORMAT };
            if (this_ver < other_ver)
                return { -1, type == SortType::PACK_FORMAT };
            break;
        }
    }
    return { 0, false };
}

bool DataPack::applyFilter(QRegularExpression filter) const
{
    if (filter.match(description()).hasMatch())
        return true;

    if (filter.match(QString::number(packFormat())).hasMatch())
        return true;

    if (filter.match(compatibleVersions().first.toString()).hasMatch())
        return true;
    if (filter.match(compatibleVersions().second.toString()).hasMatch())
        return true;

    return Resource::applyFilter(filter);
}

bool DataPack::valid() const
{
    return m_pack_format != 0;
}
