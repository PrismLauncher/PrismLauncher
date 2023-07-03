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

#include "Version.h"

// Values taken from:
// https://minecraft.fandom.com/wiki/Tutorials/Creating_a_data_pack#%22pack_format%22
static const QMap<int, std::pair<Version, Version>> s_pack_format_versions = {
    { 4, { Version("1.13"), Version("1.14.4") } },   { 5, { Version("1.15"), Version("1.16.1") } },
    { 6, { Version("1.16.2"), Version("1.16.5") } }, { 7, { Version("1.17"), Version("1.17.1") } },
    { 8, { Version("1.18"), Version("1.18.1") } },   { 9, { Version("1.18.2"), Version("1.18.2") } },
    { 10, { Version("1.19"), Version("1.19.3") } },  { 11, { Version("23w03a"), Version("23w05a") } },
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
        }
        case SortType::PACK_FORMAT: {
            auto this_ver = packFormat();
            auto other_ver = cast_other.packFormat();

            if (this_ver > other_ver)
                return { 1, type == SortType::PACK_FORMAT };
            if (this_ver < other_ver)
                return { -1, type == SortType::PACK_FORMAT };
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
