// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#pragma once
#include <QString>

/**
 * \brief The Config class holds all the build-time information passed from the build system.
 */
class Config
{
public:
    Config();
    QString LAUNCHER_NAME;
    QString LAUNCHER_DISPLAYNAME;
    QString LAUNCHER_COPYRIGHT;
    QString LAUNCHER_DOMAIN;
    QString LAUNCHER_CONFIGFILE;
    QString LAUNCHER_GIT;
    QString LAUNCHER_DESKTOPFILENAME;

    /// The major version number.
    int VERSION_MAJOR;
    /// The minor version number.
    int VERSION_MINOR;
    /// The hotfix number.
    int VERSION_HOTFIX;
    /// The build number.
    int VERSION_BUILD;

    /**
     * The version channel
     * This is used by the updater to determine what channel the current version came from.
     */
    QString VERSION_CHANNEL;

    bool UPDATER_ENABLED = false;

    /// A short string identifying this build's platform. For example, "lin64" or "win32".
    QString BUILD_PLATFORM;

    /// URL for the updater's channel
    QString UPDATER_BASE;


    /// User-Agent to use.
    QString USER_AGENT;

    /// User-Agent to use for uncached requests.
    QString USER_AGENT_UNCACHED;

    /// The git commit hash of this build
    QString GIT_COMMIT;

    /// The git refspec of this build
    QString GIT_REFSPEC;

    /// This is printed on start to standard output
    QString VERSION_STR;

    /**
     * This is used to fetch the news RSS feed.
     * It defaults in CMakeLists.txt to "https://multimc.org/rss.xml"
     */
    QString NEWS_RSS_URL;

    /**
     * URL that gets opened when the user clicks "More News"
     */
    QString NEWS_OPEN_URL;

    /**
     * URL (with arg %1 to be substituted with page-id) that gets opened when the user requests help
     */
    QString HELP_URL;

    /**
     * Client ID you can get from Imgur when you register an application
     */
    QString IMGUR_CLIENT_ID;

    /**
     * Client ID you can get from Microsoft Identity Platform when you register an application
     */
    QString MSA_CLIENT_ID;

    /**
     * Metadata repository URL prefix
     */
    QString META_URL;

    QString BUG_TRACKER_URL;
    QString TRANSLATIONS_URL;
    QString MATRIX_URL;
    QString DISCORD_URL;
    QString SUBREDDIT_URL;

    QString RESOURCE_BASE = "https://resources.download.minecraft.net/";
    QString LIBRARY_BASE = "https://libraries.minecraft.net/";
    QString AUTH_BASE = "https://authserver.mojang.com/";
    QString IMGUR_BASE_URL = "https://api.imgur.com/3/";
    QString FMLLIBS_BASE_URL = "https://files.polymc.org/fmllibs/";
    QString TRANSLATIONS_BASE_URL = "https://i18n.polymc.org/";

    QString MODPACKSCH_API_BASE_URL = "https://api.modpacks.ch/";

    QString LEGACY_FTB_CDN_BASE_URL = "https://dist.creeper.host/FTB2/";

    QString ATL_DOWNLOAD_SERVER_URL = "https://download.nodecdn.net/containers/atl/";

    QString TECHNIC_API_BASE_URL = "https://api.technicpack.net/";
    /**
     * The build that is reported to the Technic API.
     */
    QString TECHNIC_API_BUILD = "multimc";

    /**
     * \brief Converts the Version to a string.
     * \return The version number in string format (major.minor.revision.build).
     */
    QString printableVersionString() const;
};

extern const Config BuildConfig;

