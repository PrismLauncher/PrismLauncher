// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 Prism Launcher Contributors
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

#include <QJsonObject>
#include <QList>
#include <QString>

namespace TechnicPlatform {

/** Represents pack information from the Technic Platform API. */
struct PackInfo {
    QString name;
    QString displayName;
    QString url;          // Download URL for non-Solder packs
    QString platformUrl;  // Website URL
    QString minecraft;    // Minecraft version
    QString version;      // Current pack version (for non-Solder)
    QString solderUrl;    // Solder API URL if this is a Solder pack
    QString description;
    QString author;

    bool isSolder = false;
    bool isLoaded = false;
};

/** Loads pack info from a Platform API JSON response. */
void loadPackInfo(PackInfo& info, const QJsonObject& obj);

/** Represents version information from the Solder API. */
struct SolderPackInfo {
    QString name;
    QString displayName;
    QString recommended;
    QString latest;
    QList<QString> builds;

    bool isLoaded = false;
};

/** Loads Solder pack info from a Solder API JSON response. */
void loadSolderPackInfo(SolderPackInfo& info, const QJsonObject& obj);

/** Represents a mod in a Solder build. */
struct SolderMod {
    QString name;
    QString version;
    QString md5;
    QString url;
};

/** Represents a specific build from the Solder API. */
struct SolderBuild {
    QString minecraft;
    QList<SolderMod> mods;
};

/** Loads a Solder build from a Solder API JSON response. */
void loadSolderBuild(SolderBuild& build, const QJsonObject& obj);

}  // namespace TechnicPlatform
