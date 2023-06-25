// SPDX-FileCopyrightText: 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
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
 */

#include "GitHubRelease.h"

QDebug operator<<(QDebug debug, const GitHubReleaseAsset& asset)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "GitHubReleaseAsset( "
                       "id: "
                    << asset.id
                    << ", "
                       "name "
                    << asset.name
                    << ", "
                       "label: "
                    << asset.label
                    << ", "
                       "content_type: "
                    << asset.content_type
                    << ", "
                       "size: "
                    << asset.size
                    << ", "
                       "created_at: "
                    << asset.created_at
                    << ", "
                       "updated_at: "
                    << asset.updated_at
                    << ", "
                       "browser_download_url: "
                    << asset.browser_download_url
                    << " "
                       ")";
    return debug;
}

QDebug operator<<(QDebug debug, const GitHubRelease& rls)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "GitHubRelease( "
                       "id: "
                    << rls.id
                    << ", "
                       "name "
                    << rls.name
                    << ", "
                       "tag_name: "
                    << rls.tag_name
                    << ", "
                       "created_at: "
                    << rls.created_at
                    << ", "
                       "published_at: "
                    << rls.published_at
                    << ", "
                       "prerelease: "
                    << rls.prerelease
                    << ", "
                       "draft: "
                    << rls.draft
                    << ", "
                       "version"
                    << rls.version
                    << ", "
                       "body: "
                    << rls.body
                    << ", "
                       "assets: "
                    << rls.assets
                    << " "
                       ")";
    return debug;
}
