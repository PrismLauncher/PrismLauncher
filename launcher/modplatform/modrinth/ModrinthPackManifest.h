// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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
 *      Copyright 2022 kb1000
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

#include <QMetaType>

#include <QByteArray>
#include <QCryptographicHash>
#include <QQueue>
#include <QString>
#include <QUrl>
#include <QVector>

#include "modplatform/ModIndex.h"

class MinecraftInstance;

namespace Modrinth {

struct File {
    QString path;

    QCryptographicHash::Algorithm hashAlgorithm;
    QByteArray hash;
    QQueue<QUrl> downloads;
    bool required = true;
};

struct DonationData {
    QString id;
    QString platform;
    QString url;
};

struct ModpackExtra {
    QString body;

    QString projectUrl;

    QString issuesUrl;
    QString sourceUrl;
    QString wikiUrl;
    QString discordUrl;

    QList<DonationData> donate;
};

struct ModpackVersion {
    QString name;
    QString version;
    ModPlatform::IndexedVersionType version_type;
    QString changelog;

    QString id;
    QString project_id;

    QString date;

    QString download_url;
};

struct Modpack {
    QString id;

    QString name;
    QString description;
    std::tuple<QString, QUrl> author;
    QString iconName;
    QUrl iconUrl;

    bool versionsLoaded = false;
    bool extraInfoLoaded = false;

    ModpackExtra extra;
    QVector<ModpackVersion> versions;
};

void loadIndexedPack(Modpack&, QJsonObject&);
void loadIndexedInfo(Modpack&, QJsonObject&);
void loadIndexedVersions(Modpack&, QJsonDocument&);
auto loadIndexedVersion(QJsonObject&) -> ModpackVersion;

auto validateDownloadUrl(QUrl) -> bool;

}  // namespace Modrinth

Q_DECLARE_METATYPE(Modrinth::Modpack)
Q_DECLARE_METATYPE(Modrinth::ModpackVersion)
