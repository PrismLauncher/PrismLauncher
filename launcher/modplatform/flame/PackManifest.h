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
#include <QVector>
#include <QMap>
#include <QUrl>
#include <QJsonObject>

namespace Flame
{
struct File
{
    // NOTE: throws JSONValidationError
    bool parseFromObject(const QJsonObject& object);

    int projectId = 0;
    int fileId = 0;
    // NOTE: the opposite to 'optional'. This is at the time of writing unused.
    bool required = true;
    QString hash;
    // NOTE: only set on blocked files ! Empty otherwise.
    QString websiteUrl;

    // our
    bool resolved = false;
    QString fileName;
    QUrl url;
    QString targetFolder = QStringLiteral("mods");
    enum class Type
    {
        Unknown,
        Folder,
        Ctoc,
        SingleFile,
        Cmod2,
        Modpack,
        Mod
    } type = Type::Mod;
};

struct Modloader
{
    QString id;
    bool primary = false;
};

struct Minecraft
{
    QString version;
    QString libraries;
    QVector<Flame::Modloader> modLoaders;
};

struct Manifest
{
    QString manifestType;
    int manifestVersion = 0;
    Flame::Minecraft minecraft;
    QString name;
    QString version;
    QString author;
    //File id -> File
    QMap<int,Flame::File> files;
    QString overrides;
};

void loadManifest(Flame::Manifest & m, const QString &filepath);
}
