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

// Settings-key helpers for the mod profile system.
//
// All keys are namespaced by the model's directory name (e.g. "mods",
// "coremods", "nilmods") so each page type stores its profiles independently.
//
// This header is intentionally shared between the UI layer (ModFolderPage)
// and the launch layer (ScanModFolders) so the key format is defined in one
// place and cannot silently diverge.

namespace ModProfileKeys {

inline QString profileListKey(const QString& prefix)
{
    return QStringLiteral("ModProfileList_") + prefix;
}

inline QString profileKey(const QString& prefix, const QString& name)
{
    return prefix + QStringLiteral("/ModProfile_") + name;
}

// Stores the QStringList of profile names selected for runtime (launch-time) use.
// An empty list means no runtime selection is active; the launch derives the
// baseline from the last-active editing profile instead (see lastActiveIndexKey()).
inline QString runtimeProfilesKey(const QString& prefix)
{
    return QStringLiteral("ModRuntimeProfiles_") + prefix;
}

// Stores the index of the last-active editing profile tab. Shared between
// ModFolderPage (writes it on every tab switch) and ScanModFolders (reads it
// to derive the launch baseline when no runtime override is configured) so
// the key format can't silently diverge between the two.
inline QString lastActiveIndexKey(const QString& prefix)
{
    return QStringLiteral("ModProfileLastActiveIndex_") + prefix;
}

}  // namespace ModProfileKeys
