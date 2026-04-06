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
#include <utility>

#include "MTPixmapCache.h"
#include "Version.h"
#include "minecraft/mod/tasks/LocalDataPackParseTask.h"

// Values taken from:
// https://minecraft.wiki/w/Pack_format#List_of_data_pack_formats
static const QMap<std::pair<int, int>, std::pair<Version, Version>> s_pack_format_versions = {
    { { 4, 0 }, { Version("1.13"), Version("1.14.4") } },
    { { 5, 0 }, { Version("1.15"), Version("1.16.1") } },
    { { 6, 0 }, { Version("1.16.2"), Version("1.16.5") } },
    { { 7, 0 }, { Version("1.17"), Version("1.17.1") } },
    { { 8, 0 }, { Version("1.18"), Version("1.18.1") } },
    { { 9, 0 }, { Version("1.18.2"), Version("1.18.2") } },
    { { 10, 0 }, { Version("1.19"), Version("1.19.3") } },
    { { 11, 0 }, { Version("23w03a"), Version("23w05a") } },
    { { 12, 0 }, { Version("1.19.4"), Version("1.19.4") } },
    { { 13, 0 }, { Version("23w12a"), Version("23w14a") } },
    { { 14, 0 }, { Version("23w16a"), Version("23w17a") } },
    { { 15, 0 }, { Version("1.20"), Version("1.20.1") } },
    { { 16, 0 }, { Version("23w31a"), Version("23w31a") } },
    { { 17, 0 }, { Version("23w32a"), Version("23w35a") } },
    { { 18, 0 }, { Version("1.20.2"), Version("1.20.2") } },
    { { 19, 0 }, { Version("23w40a"), Version("23w40a") } },
    { { 20, 0 }, { Version("23w41a"), Version("23w41a") } },
    { { 21, 0 }, { Version("23w42a"), Version("23w42a") } },
    { { 22, 0 }, { Version("23w43a"), Version("23w43b") } },
    { { 23, 0 }, { Version("23w44a"), Version("23w44a") } },
    { { 24, 0 }, { Version("23w45a"), Version("23w45a") } },
    { { 25, 0 }, { Version("23w46a"), Version("23w46a") } },
    { { 26, 0 }, { Version("1.20.3"), Version("1.20.4") } },
    { { 27, 0 }, { Version("23w51a"), Version("23w51b") } },
    { { 28, 0 }, { Version("24w03a"), Version("24w03b") } },
    { { 29, 0 }, { Version("24w04a"), Version("24w04a") } },
    { { 30, 0 }, { Version("24w05a"), Version("24w05b") } },
    { { 31, 0 }, { Version("24w06a"), Version("24w06a") } },
    { { 32, 0 }, { Version("24w07a"), Version("24w07a") } },
    { { 33, 0 }, { Version("24w09a"), Version("24w09a") } },
    { { 34, 0 }, { Version("24w10a"), Version("24w10a") } },
    { { 35, 0 }, { Version("24w11a"), Version("24w11a") } },
    { { 36, 0 }, { Version("24w12a"), Version("24w12a") } },
    { { 37, 0 }, { Version("24w13a"), Version("24w13a") } },
    { { 38, 0 }, { Version("24w14a"), Version("24w14a") } },
    { { 39, 0 }, { Version("1.20.5-pre1"), Version("1.20.5-pre1") } },
    { { 40, 0 }, { Version("1.20.5-pre2"), Version("1.20.5-pre2") } },
    { { 41, 0 }, { Version("1.20.5"), Version("1.20.6") } },
    { { 42, 0 }, { Version("24w18a"), Version("24w18a") } },
    { { 43, 0 }, { Version("24w19a"), Version("24w19b") } },
    { { 44, 0 }, { Version("24w20a"), Version("24w20a") } },
    { { 45, 0 }, { Version("24w21a"), Version("24w21b") } },
    { { 46, 0 }, { Version("1.21-pre1"), Version("1.21-pre1") } },
    { { 47, 0 }, { Version("1.21-pre2"), Version("1.21-pre2") } },
    { { 48, 0 }, { Version("1.21"), Version("1.21.1") } },
    { { 49, 0 }, { Version("24w33a"), Version("24w33a") } },
    { { 50, 0 }, { Version("24w34a"), Version("24w34a") } },
    { { 51, 0 }, { Version("24w35a"), Version("24w35a") } },
    { { 52, 0 }, { Version("24w36a"), Version("24w36a") } },
    { { 53, 0 }, { Version("24w37a"), Version("24w37a") } },
    { { 54, 0 }, { Version("24w38a"), Version("24w38a") } },
    { { 55, 0 }, { Version("24w39a"), Version("24w39a") } },
    { { 56, 0 }, { Version("24w40a"), Version("24w40a") } },
    { { 57, 0 }, { Version("1.21.2"), Version("1.21.3") } },
    { { 58, 0 }, { Version("24w44a"), Version("24w44a") } },
    { { 59, 0 }, { Version("24w45a"), Version("24w45a") } },
    { { 60, 0 }, { Version("24w46a"), Version("1.21.4-pre1") } },
    { { 61, 0 }, { Version("1.21.4"), Version("1.21.4") } },
    { { 62, 0 }, { Version("25w02a"), Version("25w02a") } },
    { { 63, 0 }, { Version("25w03a"), Version("25w03a") } },
    { { 64, 0 }, { Version("25w04a"), Version("25w04a") } },
    { { 65, 0 }, { Version("25w05a"), Version("25w05a") } },
    { { 66, 0 }, { Version("25w06a"), Version("25w06a") } },
    { { 67, 0 }, { Version("25w07a"), Version("25w07a") } },
    { { 68, 0 }, { Version("25w08a"), Version("25w08a") } },
    { { 69, 0 }, { Version("25w09a"), Version("25w09b") } },
    { { 70, 0 }, { Version("25w10a"), Version("1.21.5-pre1") } },
    { { 71, 0 }, { Version("1.21.5"), Version("1.21.5") } },
    { { 72, 0 }, { Version("25w15a"), Version("25w15a") } },
    { { 73, 0 }, { Version("25w16a"), Version("25w16a") } },
    { { 74, 0 }, { Version("25w17a"), Version("25w17a") } },
    { { 75, 0 }, { Version("25w18a"), Version("25w18a") } },
    { { 76, 0 }, { Version("25w19a"), Version("25w19a") } },
    { { 77, 0 }, { Version("25w20a"), Version("25w20a") } },
    { { 78, 0 }, { Version("25w21a"), Version("25w21a") } },
    { { 79, 0 }, { Version("1.21.6-pre1"), Version("1.21.6-pre2") } },
    { { 80, 0 }, { Version("1.21.6"), Version("1.21.6") } },
    { { 81, 0 }, { Version("1.21.7"), Version("1.21.8") } },
    { { 82, 0 }, { Version("25w31a"), Version("25w31a") } },
    { { 83, 0 }, { Version("25w32a"), Version("25w32a") } },
    { { 83, 1 }, { Version("25w33a"), Version("25w33a") } },
    { { 84, 0 }, { Version("25w34a"), Version("25w34b") } },
    { { 85, 0 }, { Version("25w35a"), Version("25w35a") } },
    { { 86, 0 }, { Version("25w36a"), Version("25w36b") } },
    { { 87, 0 }, { Version("25w37a"), Version("1.21.9-pre1") } },
    { { 87, 1 }, { Version("1.21.9-pre1"), Version("1.21.9-pre1") } },
    { { 88, 0 }, { Version("1.21.9"), Version("1.21.10") } },
    { { 89, 0 }, { Version("25w41a"), Version("25w41a") } },
    { { 90, 0 }, { Version("25w42a"), Version("25w42a") } },
    { { 91, 0 }, { Version("25w43a"), Version("25w43a") } },
    { { 92, 0 }, { Version("25w44a"), Version("25w44a") } },
    { { 93, 0 }, { Version("25w45a"), Version("25w45a") } },
    { { 93, 1 }, { Version("25w46a"), Version("25w46a") } },
    { { 94, 0 }, { Version("1.21.11-pre1"), Version("1.21.11-pre3") } },
    { { 94, 1 }, { Version("1.21.11-pre4"), Version("1.21.11") } },
    { { 95, 0 }, { Version("26.1-snap1"), Version("26.1-snap1") } },
    { { 96, 0 }, { Version("26.1-snap2"), Version("26.1-snap2") } },
    { { 97, 0 }, { Version("26.1-snap3"), Version("26.1-snap3") } },
    { { 97, 1 }, { Version("26.1-snap4"), Version("26.1-snap4") } },
    { { 98, 0 }, { Version("26.1-snap5"), Version("26.1-snap5") } },
    { { 99, 0 }, { Version("26.1-snap6"), Version("26.1-snap6") } },
    { { 99, 1 }, { Version("26.1-snap7"), Version("26.1-snap7") } },
    { { 99, 2 }, { Version("26.1-snap8"), Version("26.1-snap9") } },
    { { 99, 3 }, { Version("26.1-snap10"), Version("26.1-snap10") } },
    { { 100, 0 }, { Version("26.1-snap11"), Version("26.1-snap11") } },
};

void DataPack::setPackFormat(int new_format_id, std::pair<int, int> min_format, std::pair<int, int> max_format)
{
    QMutexLocker locker(&m_data_lock);

    m_pack_format = new_format_id;
    m_min_format = min_format;
    m_max_format = max_format;
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
        qDebug() << "Data Pack" << name() << "Had it's image evicted from the cache. reloading...";
        PixmapCache::markCacheMissByEviciton();
    }

    // Imaged got evicted from the cache. Re-process it and retry.
    DataPackUtils::processPackPNG(this);
    return image(size);
}

static std::pair<Version, Version> map(std::pair<int, int> format, const QMap<std::pair<int, int>, std::pair<Version, Version>>& versions)
{
    if (format.first == 0 || !versions.contains(format)) {
        return { {}, {} };
    }
    return versions.constFind(format).value();
}
static std::pair<Version, Version> map(int format, const QMap<std::pair<int, int>, std::pair<Version, Version>>& versions)
{
    return map({ format, 0 }, versions);
}

int DataPack::compare(const Resource& other, SortType type) const
{
    const auto& cast_other = static_cast<const DataPack&>(other);
    if (type == SortType::PACK_FORMAT) {
        auto this_ver = packFormat();
        auto other_ver = cast_other.packFormat();

        if (this_ver > other_ver)
            return 1;
        if (this_ver < other_ver)
            return -1;
    } else {
        return Resource::compare(other, type);
    }
    return 0;
}

bool DataPack::applyFilter(QRegularExpression filter) const
{
    if (filter.match(description()).hasMatch()) {
        return true;
    }

    if (filter.match(QString::number(packFormat())).hasMatch()) {
        return true;
    }

    auto versions = { map(m_pack_format, mappings()), map(m_min_format, mappings()), map(m_max_format, mappings()) };
    for (const auto& version : versions) {
        if (!version.first.isEmpty()) {
            if (filter.match(version.first.toString()).hasMatch()) {
                return true;
            }
            if (filter.match(version.second.toString()).hasMatch()) {
                return true;
            }
        }
    }
    return Resource::applyFilter(filter);
}

bool DataPack::valid() const
{
    return m_pack_format != 0 || (m_min_format.first != 0 && m_max_format.first != 0);
}

QMap<std::pair<int, int>, std::pair<Version, Version>> DataPack::mappings() const
{
    return s_pack_format_versions;
}

QString DataPack::packFormatStr() const
{
    if (m_pack_format != 0) {
        auto version_bounds = map(m_pack_format, mappings());
        if (version_bounds.first.toString().isEmpty()) {
            return QString::number(m_pack_format);
        }
        return QString("%1 (%2 - %3)")
            .arg(QString::number(m_pack_format), version_bounds.first.toString(), version_bounds.second.toString());
    }
    auto min_bound = map(m_min_format, mappings());
    auto max_bound = map(m_max_format, mappings());
    auto min_version = min_bound.first;
    auto max_version = max_bound.second;
    if (min_version.isEmpty() || max_version.isEmpty()) {
        return tr("Unrecognized");
    }
    auto str = QString("[") + QString::number(m_min_format.first);
    if (m_min_format.second != 0) {
        str += "." + QString::number(m_min_format.second);
    }

    str += QString(" - ") + QString::number(m_max_format.first);
    if (m_max_format.second != 0) {
        str += "." + QString::number(m_max_format.second);
    }

    return str + QString(" (%2 - %3)").arg(min_version.toString(), max_version.toString());
}
