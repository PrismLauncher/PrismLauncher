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

#include <QList>
#include <QString>

#include "net/NetJob.h"
#include "tasks/Task.h"

namespace Technic {

class API : public QObject {
    Q_OBJECT
   public:
    /** Fetches platform pack info from the Technic API.
     *  This includes the pack's metadata, download URL, and whether it uses Solder.
     */
    static Task::Ptr getPackInfo(const QString& packSlug, QByteArray* response);

    /** Fetches Solder pack info including version list.
     *  Only applicable for packs that use Solder.
     */
    static Task::Ptr getSolderPackInfo(const QString& solderUrl, const QString& packSlug, QByteArray* response);

    /** Fetches a specific build from a Solder pack.
     *  Returns the list of mods for that build.
     */
    static Task::Ptr getSolderPackBuild(const QString& solderUrl, const QString& packSlug, const QString& build, QByteArray* response);

    /** Returns the Platform API URL for a pack. */
    static QString getPlatformPackUrl(const QString& packSlug);

    /** Returns the Solder API URL for pack info. */
    static QString getSolderPackUrl(const QString& solderUrl, const QString& packSlug);

    /** Returns the Solder API URL for a specific build. */
    static QString getSolderBuildUrl(const QString& solderUrl, const QString& packSlug, const QString& build);
};

}  // namespace Technic
