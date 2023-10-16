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

#pragma once
#include <QDateTime>
#include <QList>
#include <QString>

#include <QDebug>

#include "Version.h"

struct GitHubReleaseAsset {
    int id = -1;
    QString name;
    QString label;
    QString content_type;
    int size;
    QDateTime created_at;
    QDateTime updated_at;
    QString browser_download_url;

    bool isValid() { return id > 0; }
};

struct GitHubRelease {
    int id = -1;
    QString name;
    QString tag_name;
    QDateTime created_at;
    QDateTime published_at;
    bool prerelease;
    bool draft;
    QString body;
    QList<GitHubReleaseAsset> assets;
    Version version;

    bool isValid() const { return id > 0; }
};

QDebug operator<<(QDebug debug, const GitHubReleaseAsset& rls);
QDebug operator<<(QDebug debug, const GitHubRelease& rls);
