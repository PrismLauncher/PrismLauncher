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
#include "api/structures/Provider.h"

namespace Platform {

enum class DependencyType { REQUIRED, OPTIONAL, INCOMPATIBLE, EMBEDDED, TOOL, INCLUDE, UNKNOWN };

struct Dependency {
    QVariant addonId;
    DependencyType type;
    QString version;
};

struct OverrideDep {
    QString quilt;
    QString fabric;
    QString slug;
    Provider provider;
};

inline auto getOverrideDeps() -> QList<OverrideDep>
{
    return { { "634179", "306612", "API", Provider::FLAME },
             { "720410", "308769", "KotlinLibraries", Provider::FLAME },

             { "qvIfYCYJ", "P7dR8mSH", "API", Provider::MODRINTH },
             { "lwVhp9o5", "Ha28R6CL", "KotlinLibraries", Provider::MODRINTH } };
}

}  // namespace Platform
