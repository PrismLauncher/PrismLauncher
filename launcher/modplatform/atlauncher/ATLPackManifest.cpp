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
 *      Copyright 2020-2021 Jamie Mansfield <jmansfield@cadixdev.org>
 *      Copyright 2021 Petr Mrazek <peterix@gmail.com>
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

#include "ATLPackManifest.h"

#include "Json.h"

static ATLauncher::DownloadType parseDownloadType(QString rawType)
{
    if (rawType == QString("server")) {
        return ATLauncher::DownloadType::Server;
    } else if (rawType == QString("browser")) {
        return ATLauncher::DownloadType::Browser;
    } else if (rawType == QString("direct")) {
        return ATLauncher::DownloadType::Direct;
    }

    return ATLauncher::DownloadType::Unknown;
}

static ATLauncher::ModType parseModType(QString rawType)
{
    // See https://wiki.atlauncher.com/mod_types
    if (rawType == QString("root")) {
        return ATLauncher::ModType::Root;
    } else if (rawType == QString("forge")) {
        return ATLauncher::ModType::Forge;
    } else if (rawType == QString("jar")) {
        return ATLauncher::ModType::Jar;
    } else if (rawType == QString("mods")) {
        return ATLauncher::ModType::Mods;
    } else if (rawType == QString("flan")) {
        return ATLauncher::ModType::Flan;
    } else if (rawType == QString("dependency") || rawType == QString("depandency")) {
        return ATLauncher::ModType::Dependency;
    } else if (rawType == QString("ic2lib")) {
        return ATLauncher::ModType::Ic2Lib;
    } else if (rawType == QString("denlib")) {
        return ATLauncher::ModType::DenLib;
    } else if (rawType == QString("coremods")) {
        return ATLauncher::ModType::Coremods;
    } else if (rawType == QString("mcpc")) {
        return ATLauncher::ModType::MCPC;
    } else if (rawType == QString("plugins")) {
        return ATLauncher::ModType::Plugins;
    } else if (rawType == QString("extract")) {
        return ATLauncher::ModType::Extract;
    } else if (rawType == QString("decomp")) {
        return ATLauncher::ModType::Decomp;
    } else if (rawType == QString("texturepack")) {
        return ATLauncher::ModType::TexturePack;
    } else if (rawType == QString("resourcepack")) {
        return ATLauncher::ModType::ResourcePack;
    } else if (rawType == QString("shaderpack")) {
        return ATLauncher::ModType::ShaderPack;
    } else if (rawType == QString("texturepackextract")) {
        return ATLauncher::ModType::TexturePackExtract;
    } else if (rawType == QString("resourcepackextract")) {
        return ATLauncher::ModType::ResourcePackExtract;
    } else if (rawType == QString("millenaire")) {
        return ATLauncher::ModType::Millenaire;
    }

    return ATLauncher::ModType::Unknown;
}

static void loadVersionLoader(ATLauncher::VersionLoader& p, QJsonObject& obj)
{
    p.type = Json::requireString(obj, "type");
    p.choose = obj["choose"].toBool();

    auto metadata = Json::requireObject(obj, "metadata");
    p.latest = metadata["latest"].toBool();
    p.recommended = metadata["recommended"].toBool();

    // Minecraft Forge
    if (p.type == "forge" || p.type == "neoforge") {
        p.version = metadata["version"].toString("");
    }

    // Fabric Loader
    if (p.type == "fabric") {
        p.version = metadata["loader"].toString("");
    }
}

static void loadVersionLibrary(ATLauncher::VersionLibrary& p, QJsonObject& obj)
{
    p.url = Json::requireString(obj, "url");
    p.file = Json::requireString(obj, "file");
    p.md5 = Json::requireString(obj, "md5");

    p.download_raw = Json::requireString(obj, "download");
    p.download = parseDownloadType(p.download_raw);

    p.server = obj["server"].toString("");
}

static void loadVersionConfigs(ATLauncher::VersionConfigs& p, QJsonObject& obj)
{
    p.filesize = Json::requireInteger(obj, "filesize");
    p.sha1 = Json::requireString(obj, "sha1");
}

static void loadVersionMod(ATLauncher::VersionMod& p, QJsonObject& obj)
{
    p.name = Json::requireString(obj, "name");
    p.version = Json::requireString(obj, "version");
    p.url = Json::requireString(obj, "url");
    p.file = Json::requireString(obj, "file");
    p.md5 = obj["md5"].toString("");

    p.download_raw = Json::requireString(obj, "download");
    p.download = parseDownloadType(p.download_raw);

    p.type_raw = Json::requireString(obj, "type");
    p.type = parseModType(p.type_raw);

    // This contributes to the Minecraft Forge detection, where we rely on mod.type being "Forge"
    // when the mod represents Forge. As there is little difference between "Jar" and "Forge, some
    // packs regretfully use "Jar". This will correct the type to "Forge" in these cases (as best
    // it can).
    if (p.name == QString("Minecraft Forge") && p.type == ATLauncher::ModType::Jar) {
        p.type_raw = "forge";
        p.type = ATLauncher::ModType::Forge;
    }

    if (obj.contains("extractTo")) {
        p.extractTo_raw = Json::requireString(obj, "extractTo");
        p.extractTo = parseModType(p.extractTo_raw);
        p.extractFolder = obj["extractFolder"].toString("").replace("%s%", "/");
    }

    if (obj.contains("decompType")) {
        p.decompType_raw = Json::requireString(obj, "decompType");
        p.decompType = parseModType(p.decompType_raw);
        p.decompFile = Json::requireString(obj, "decompFile");
    }

    p.description = obj["description"].toString("");
    p.optional = obj["optional"].toBool();
    p.recommended = obj["recommended"].toBool();
    p.selected = obj["selected"].toBool();
    p.hidden = obj["hidden"].toBool();
    p.library = obj["library"].toBool();
    p.group = obj["group"].toString("");
    if (obj.contains("depends")) {
        auto dependsArr = Json::requireArray(obj, "depends");
        for (const auto depends : dependsArr) {
            p.depends.append(Json::requireString(depends));
        }
    }
    p.colour = obj["colour"].toString("");
    p.warning = obj["warning"].toString("");

    p.client = obj["client"].toBool();

    // computed
    p.effectively_hidden = p.hidden || p.library;
}

static void loadVersionMessages(ATLauncher::VersionMessages& m, QJsonObject& obj)
{
    m.install = obj["install"].toString("");
    m.update = obj["update"].toString("");
}

static void loadVersionMainClass(ATLauncher::PackVersionMainClass& m, QJsonObject& obj)
{
    m.mainClass = obj["mainClass"].toString("");
    m.depends = obj["depends"].toString("");
}

static void loadVersionExtraArguments(ATLauncher::PackVersionExtraArguments& a, QJsonObject& obj)
{
    a.arguments = obj["arguments"].toString("");
    a.depends = obj["depends"].toString("");
}

static void loadVersionKeep(ATLauncher::VersionKeep& k, QJsonObject& obj)
{
    k.base = Json::requireString(obj, "base");
    k.target = Json::requireString(obj, "target");
}

static void loadVersionKeeps(ATLauncher::VersionKeeps& k, QJsonObject& obj)
{
    if (obj.contains("files")) {
        auto files = Json::requireArray(obj, "files");
        for (const auto keepRaw : files) {
            auto keepObj = Json::requireObject(keepRaw);
            ATLauncher::VersionKeep keep;
            loadVersionKeep(keep, keepObj);
            k.files.append(keep);
        }
    }

    if (obj.contains("folders")) {
        auto folders = Json::requireArray(obj, "folders");
        for (const auto keepRaw : folders) {
            auto keepObj = Json::requireObject(keepRaw);
            ATLauncher::VersionKeep keep;
            loadVersionKeep(keep, keepObj);
            k.folders.append(keep);
        }
    }
}

static void loadVersionDelete(ATLauncher::VersionDelete& d, QJsonObject& obj)
{
    d.base = Json::requireString(obj, "base");
    d.target = Json::requireString(obj, "target");
}

static void loadVersionDeletes(ATLauncher::VersionDeletes& d, QJsonObject& obj)
{
    if (obj.contains("files")) {
        auto files = Json::requireArray(obj, "files");
        for (const auto deleteRaw : files) {
            auto deleteObj = Json::requireObject(deleteRaw);
            ATLauncher::VersionDelete versionDelete;
            loadVersionDelete(versionDelete, deleteObj);
            d.files.append(versionDelete);
        }
    }

    if (obj.contains("folders")) {
        auto folders = Json::requireArray(obj, "folders");
        for (const auto deleteRaw : folders) {
            auto deleteObj = Json::requireObject(deleteRaw);
            ATLauncher::VersionDelete versionDelete;
            loadVersionDelete(versionDelete, deleteObj);
            d.folders.append(versionDelete);
        }
    }
}

void ATLauncher::loadVersion(PackVersion& v, QJsonObject& obj)
{
    v.version = Json::requireString(obj, "version");
    v.minecraft = Json::requireString(obj, "minecraft");
    v.noConfigs = obj["noConfigs"].toBool();

    if (obj.contains("mainClass")) {
        auto main = Json::requireObject(obj, "mainClass");
        loadVersionMainClass(v.mainClass, main);
    }

    if (obj.contains("extraArguments")) {
        auto arguments = Json::requireObject(obj, "extraArguments");
        loadVersionExtraArguments(v.extraArguments, arguments);
    }

    if (obj.contains("loader")) {
        auto loader = Json::requireObject(obj, "loader");
        loadVersionLoader(v.loader, loader);
    }

    if (obj.contains("libraries")) {
        auto libraries = Json::requireArray(obj, "libraries");
        for (const auto libraryRaw : libraries) {
            auto libraryObj = Json::requireObject(libraryRaw);
            ATLauncher::VersionLibrary target;
            loadVersionLibrary(target, libraryObj);
            v.libraries.append(target);
        }
    }

    if (obj.contains("mods")) {
        auto mods = Json::requireArray(obj, "mods");
        for (const auto modRaw : mods) {
            auto modObj = Json::requireObject(modRaw);
            ATLauncher::VersionMod mod;
            loadVersionMod(mod, modObj);
            v.mods.append(mod);
        }
    }

    if (obj.contains("configs")) {
        auto configsObj = Json::requireObject(obj, "configs");
        loadVersionConfigs(v.configs, configsObj);
    }

    auto colourObj = obj["colours"].toObject();
    for (const auto& key : colourObj.keys()) {
        v.colours[key] = Json::requireString(colourObj.value(key), "colour");
    }

    auto warningsObj = obj["warnings"].toObject();
    for (const auto& key : warningsObj.keys()) {
        v.warnings[key] = Json::requireString(warningsObj.value(key), "warning");
    }

    auto messages = obj["messages"].toObject();
    loadVersionMessages(v.messages, messages);

    auto keeps = obj["keeps"].toObject();
    loadVersionKeeps(v.keeps, keeps);

    auto deletes = obj["deletes"].toObject();
    loadVersionDeletes(v.deletes, deletes);
}
