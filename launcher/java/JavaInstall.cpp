// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023-2024 Trial97 <alexandru.tripon97@gmail.com>
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

#include "JavaInstall.h"

#include "BaseVersion.h"
#include "StringUtils.h"

bool JavaInstall::operator<(const JavaInstall& rhs)
{
    auto archCompare = StringUtils::naturalCompare(arch, rhs.arch, Qt::CaseInsensitive);
    if (archCompare != 0)
        return archCompare < 0;
    if (id < rhs.id) {
        return true;
    }
    if (id > rhs.id) {
        return false;
    }
    return StringUtils::naturalCompare(path, rhs.path, Qt::CaseInsensitive) < 0;
}

bool JavaInstall::operator==(const JavaInstall& rhs)
{
    return arch == rhs.arch && id == rhs.id && path == rhs.path;
}

bool JavaInstall::operator>(const JavaInstall& rhs)
{
    return (!operator<(rhs)) && (!operator==(rhs));
}

bool JavaInstall::operator<(BaseVersion& a)
{
    try {
        return operator<(dynamic_cast<JavaInstall&>(a));
    } catch (const std::bad_cast& e) {
        return BaseVersion::operator<(a);
    }
}

bool JavaInstall::operator>(BaseVersion& a)
{
    try {
        return operator>(dynamic_cast<JavaInstall&>(a));
    } catch (const std::bad_cast& e) {
        return BaseVersion::operator>(a);
    }
}
