// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (c) 2023-2025 Trial97 <alexandru.tripon97@gmail.com>
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
#include <QVariant>
#include "api/structures/Dependency.h"
#include "api/structures/ModLoader.h"
#include "api/structures/Side.h"
#include "api/structures/VersionType.h"

namespace Platform {

struct Version {
    QVariant projectId;
    QVariant fileId;
    QString version;
    QString version_number = {};
    VersionType version_type;
    QStringList mcVersion;
    QString downloadUrl;
    QString date;
    QString fileName;
    ModLoaders loaders = {};
    QString hash_type;
    QString hash;
    bool is_preferred = true;
    QString changelog;
    QList<Dependency> dependencies;
    Platform::Side side;  // this is for flame API

    // For internal use, not provided by APIs
    bool is_currently_selected = false;
};
}  // namespace Platform
