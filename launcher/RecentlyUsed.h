// SPDX-FileCopyrightText: 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
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
#pragma once

#include <QString>
#include <QUrl>

namespace RecentlyUsed {

struct RecentlyUsedData {
    /**
     * @brief Title of bookmark
     */
    const QString& title;
    /**
     * @brief Description of bookmark
     */
    const QString& desc;
    /**
     * @brief local Url of icon file
     */
    const QUrl& iconUrl;
};

/**
 * @brief platform agnostic method of saving a resource Url as recently used
 */
bool recordRecentlyUsed(const QUrl& url, RecentlyUsedData data);

}  // namespace RecentlyUsed
