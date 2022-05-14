/* Copyright 2022 kb1000
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QMetaType>

#include <QByteArray>
#include <QCryptographicHash>
#include <QString>
#include <QUrl>

class MinecraftInstance;

namespace Modrinth {

struct File
{
    QString path;

    QCryptographicHash::Algorithm hashAlgorithm;
    QByteArray hash;
    // TODO: should this support multiple download URLs, like the JSON does?
    QUrl download;
};

struct ModpackExtra {
    QString body;

    QString sourceUrl;
    QString wikiUrl;
};

struct ModpackVersion {
    QString name;
    QString version;

    QString id;
    QString project_id;

    QString date;

    QString download_url;
};

struct Modpack {
    QString id;

    QString name;
    QString description;
    QStringList authors;
    QString iconName;
    QUrl    iconUrl;

    bool    versionsLoaded = false;
    bool    extraInfoLoaded = false;

    ModpackExtra extra;
    QVector<ModpackVersion> versions;
};

void loadIndexedPack(Modpack&, QJsonObject&);
void loadIndexedInfo(Modpack&, QJsonObject&);
void loadIndexedVersions(Modpack&, QJsonDocument&);
auto loadIndexedVersion(QJsonObject&) -> ModpackVersion;

}

Q_DECLARE_METATYPE(Modrinth::Modpack);
Q_DECLARE_METATYPE(Modrinth::ModpackVersion);
