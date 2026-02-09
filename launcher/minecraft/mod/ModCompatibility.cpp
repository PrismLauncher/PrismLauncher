// SPDX-FileCopyrightText: 2026 abhicommands <114682464+abhicommands@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 abhicommands <114682464+abhicommands@users.noreply.github.com>
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

#include "ModCompatibility.h"

#include "minecraft/mod/Resource.h"

namespace {

[[nodiscard]] bool supportsVersionPattern(const QString& pattern, const QString& instanceMinecraftVersion)
{
    auto trimmedPattern = pattern.trimmed();
    auto trimmedInstanceVersion = instanceMinecraftVersion.trimmed();
    if (trimmedPattern.isEmpty() || trimmedInstanceVersion.isEmpty()) {
        return false;
    }

    if (trimmedPattern == "*") {
        return true;
    }

    if (trimmedPattern.compare(trimmedInstanceVersion, Qt::CaseInsensitive) == 0) {
        return true;
    }

    if (trimmedPattern.endsWith(".x", Qt::CaseInsensitive)) {
        auto prefix = trimmedPattern.left(trimmedPattern.size() - 2);
        return trimmedInstanceVersion.startsWith(prefix + ".", Qt::CaseInsensitive);
    }

    if (trimmedPattern.endsWith(".*", Qt::CaseInsensitive)) {
        auto prefix = trimmedPattern.left(trimmedPattern.size() - 2);
        return trimmedInstanceVersion.startsWith(prefix + ".", Qt::CaseInsensitive);
    }

    return false;
}

}  // namespace

namespace ModCompatibility {

bool supportsMinecraftVersion(const QStringList& supportedVersions, const QString& instanceMinecraftVersion)
{
    if (supportedVersions.isEmpty() || instanceMinecraftVersion.trimmed().isEmpty()) {
        return true;
    }

    for (auto const& supportedVersion : supportedVersions) {
        if (supportsVersionPattern(supportedVersion, instanceMinecraftVersion)) {
            return true;
        }
    }

    return false;
}

bool isIncompatibleWithInstanceVersion(const Resource& resource, const QString& instanceMinecraftVersion)
{
    if (!resource.enabled()) {
        return false;
    }

    auto metadata = resource.metadata();
    if (!metadata) {
        return false;
    }

    return !supportsMinecraftVersion(metadata->mcVersions, instanceMinecraftVersion);
}

}  // namespace ModCompatibility
