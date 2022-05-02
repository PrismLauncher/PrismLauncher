/*
 * Copyright 2020 Jamie Mansfield <jmansfield@cadixdev.org>
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

#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QVector>

namespace ATLauncher
{

enum class PackType
{
    Public,
    Private
};

enum class ModType
{
    Root,
    Forge,
    Jar,
    Mods,
    Flan,
    Dependency,
    Ic2Lib,
    DenLib,
    Coremods,
    MCPC,
    Plugins,
    Extract,
    Decomp,
    TexturePack,
    ResourcePack,
    ShaderPack,
    TexturePackExtract,
    ResourcePackExtract,
    Millenaire,
    Unknown
};

enum class DownloadType
{
    Server,
    Browser,
    Direct,
    Unknown
};

struct VersionLoader
{
    QString type;
    bool latest;
    bool recommended;
    bool choose;

    QString version;
};

struct VersionLibrary
{
    QString url;
    QString file;
    QString server;
    QString md5;
    DownloadType download;
    QString download_raw;
};

struct VersionMod
{
    QString name;
    QString version;
    QString url;
    QString file;
    QString md5;
    DownloadType download;
    QString download_raw;
    ModType type;
    QString type_raw;

    ModType extractTo;
    QString extractTo_raw;
    QString extractFolder;

    ModType decompType;
    QString decompType_raw;
    QString decompFile;

    QString description;
    bool optional;
    bool recommended;
    bool selected;
    bool hidden;
    bool library;
    QString group;
    QVector<QString> depends;
    QString colour;

    bool client;

    // computed
    bool effectively_hidden;
};

struct VersionConfigs
{
    int filesize;
    QString sha1;
};

struct PackVersion
{
    QString version;
    QString minecraft;
    bool noConfigs;
    QString mainClass;
    QString extraArguments;

    VersionLoader loader;
    QVector<VersionLibrary> libraries;
    QVector<VersionMod> mods;
    VersionConfigs configs;

    QMap<QString, QString> colours;
};

void loadVersion(PackVersion & v, QJsonObject & obj);

}
