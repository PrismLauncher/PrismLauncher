// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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



namespace ModProfileKeys {

inline QString profileListKey(const QString& prefix)
{
    return QStringLiteral("ModProfileList_") + prefix;
}

inline QString profileKey(const QString& prefix, const QString& name)
{
    return prefix + QStringLiteral("/ModProfile_") + name;
}


inline QString runtimeProfilesKey(const QString& prefix)
{
    return QStringLiteral("ModRuntimeProfiles_") + prefix;
}


inline QString lastActiveIndexKey(const QString& prefix)
{
    return QStringLiteral("ModProfileLastActiveIndex_") + prefix;
}

}  // namespace ModProfileKeys
