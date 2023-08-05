// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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
 *      Copyright 2020 Jamie Mansfield <jmansfield@cadixdev.org>
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

#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QVector>

namespace ATLauncher {

enum class PackType { Public, Private };

enum class ModType {
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

enum class DownloadType { Server, Browser, Direct, Unknown };

struct VersionLoader {
    QString type;
    bool latest;
    bool recommended;
    bool choose;

    QString version;
};

struct VersionLibrary {
    QString url;
    QString file;
    QString server;
    QString md5;
    DownloadType download;
    QString download_raw;
};

struct VersionMod {
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
    QString warning;

    bool client;

    // computed
    bool effectively_hidden;
};

struct VersionConfigs {
    int filesize;
    QString sha1;
};

struct VersionMessages {
    QString install;
    QString update;
};

struct VersionKeep {
    QString base;
    QString target;
};

struct VersionKeeps {
    QVector<VersionKeep> files;
    QVector<VersionKeep> folders;
};

struct VersionDelete {
    QString base;
    QString target;
};

struct VersionDeletes {
    QVector<VersionDelete> files;
    QVector<VersionDelete> folders;
};

struct PackVersionMainClass {
    QString mainClass;
    QString depends;
};

struct PackVersionExtraArguments {
    QString arguments;
    QString depends;
};

struct PackVersion {
    QString version;
    QString minecraft;
    bool noConfigs;
    PackVersionMainClass mainClass;
    PackVersionExtraArguments extraArguments;

    VersionLoader loader;
    QVector<VersionLibrary> libraries;
    QVector<VersionMod> mods;
    VersionConfigs configs;

    QMap<QString, QString> colours;
    QMap<QString, QString> warnings;
    VersionMessages messages;

    VersionKeeps keeps;
    VersionDeletes deletes;
};

void loadVersion(PackVersion& v, QJsonObject& obj);

}  // namespace ATLauncher
