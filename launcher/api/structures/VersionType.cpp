// SPDX-FileCopyrightText: 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
 *  Copyright (c) 2025 Trial97 <alexandru.tripon97@gmail.com>
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

#include "VersionType.h"

#include <QHash>

namespace Platform::VersionTypeUtils {
static const QHash<QString, VersionType> s_indexed_version_type_names = { { "release", VersionType::Release },
                                                                          { "beta", VersionType::Beta },
                                                                          { "alpha", VersionType::Alpha } };
QString toString(VersionType versionType)
{
    return s_indexed_version_type_names.key(versionType, "unknown");
}
VersionType fromString(QString versionType)
{
    return s_indexed_version_type_names.value(versionType, VersionType::Unknown);
}
}  // namespace Platform::VersionTypeUtils
