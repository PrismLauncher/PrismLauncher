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
 *      Copyright 2020 Jamie Mansfield <jmansfield@cadixdev.org>
 *      Copyright 2020-2021 Petr Mrazek <peterix@gmail.com>
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

#include "FTBPackManifest.h"

#include "Json.h"

static Result<> loadSpecs(FTB::Specs& s, const QJsonObject& obj)
{
    TRY_INTO(s.id, Json::requireInteger(obj, "id"))
    TRY_INTO(s.minimum, Json::requireInteger(obj, "minimum"))
    TRY_INTO(s.recommended, Json::requireInteger(obj, "recommended"))
    return {};
}

static Result<> loadTag(FTB::Tag& t, const QJsonObject& obj)
{
    TRY_INTO(t.id, Json::requireInteger(obj, "id"))
    TRY_INTO(t.name, Json::requireString(obj, "name"))
    return {};
}

static Result<> loadArt(FTB::Art& a, const QJsonObject& obj)
{
    TRY_INTO(a.id, Json::requireInteger(obj, "id"))
    TRY_INTO(a.url, Json::requireString(obj, "url"))
    TRY_INTO(a.type, Json::requireString(obj, "type"))
    TRY_INTO(a.width, Json::requireInteger(obj, "width"))
    TRY_INTO(a.height, Json::requireInteger(obj, "height"))
    TRY_INTO(a.compressed, Json::requireBoolean(obj, "compressed"))
    TRY_INTO(a.sha1, Json::requireString(obj, "sha1"))
    a.size = obj["size"].toInt();
    TRY_INTO(a.updated, Json::requireInteger(obj, "updated"))
    return {};
}

static Result<> loadAuthor(FTB::Author& a, const QJsonObject& obj)
{
    TRY_INTO(a.id, Json::requireInteger(obj, "id"))
    TRY_INTO(a.name, Json::requireString(obj, "name"))
    TRY_INTO(a.type, Json::requireString(obj, "type"))
    TRY_INTO(a.website, Json::requireString(obj, "website"))
    TRY_INTO(a.updated, Json::requireInteger(obj, "updated"))
    return {};
}

static Result<> loadVersionInfo(FTB::VersionInfo& v, const QJsonObject& obj)
{
    TRY_INTO(v.id, Json::requireInteger(obj, "id"))
    TRY_INTO(v.name, Json::requireString(obj, "name"))
    TRY_INTO(v.type, Json::requireString(obj, "type"))
    TRY_INTO(v.updated, Json::requireInteger(obj, "updated"))
    auto specs = Json::requireObject(obj, "specs");
    TRY(specs)
    TRY(loadSpecs(v.specs, specs.value()))
    return {};
}

Result<> FTB::loadModpack(FTB::Modpack& m, const QJsonObject& obj)
{
    TRY_INTO(m.id, Json::requireInteger(obj, "id"))
    TRY_INTO(m.name, Json::requireString(obj, "name"))
    QString name;
    TRY_INTO(name, Json::requireString(obj, "name"))
    m.safeName = name.replace(QRegularExpression("[^A-Za-z0-9]"), "").toLower() + ".png";
    TRY_INTO(m.synopsis, Json::requireString(obj, "synopsis"))
    TRY_INTO(m.description, Json::requireString(obj, "description"))
    TRY_INTO(m.type, Json::requireString(obj, "type"))
    TRY_INTO(m.featured, Json::requireBoolean(obj, "featured"))
    TRY_INTO(m.installs, Json::requireInteger(obj, "installs"))
    TRY_INTO(m.plays, Json::requireInteger(obj, "plays"))
    TRY_INTO(m.updated, Json::requireInteger(obj, "updated"))
    m.refreshed = obj["refreshed"].toInt();
    auto artArr = Json::requireArray(obj, "art");
    TRY(artArr)
    for (QJsonValueRef artRaw : artArr.value()) {
        auto artObj = Json::requireObject(artRaw);
        TRY(artObj)
        FTB::Art art;
        TRY(loadArt(art, artObj.value()))
        m.art.append(art);
    }
    auto authorArr = Json::requireArray(obj, "authors");
    TRY(authorArr)
    for (QJsonValueRef authorRaw : authorArr.value()) {
        auto authorObj = Json::requireObject(authorRaw);
        TRY(authorObj)
        FTB::Author author;
        TRY(loadAuthor(author, authorObj.value()))
        m.authors.append(author);
    }
    auto versionArr = Json::requireArray(obj, "versions");
    TRY(versionArr)
    for (QJsonValueRef versionRaw : versionArr.value()) {
        auto versionObj = Json::requireObject(versionRaw);
        TRY(versionObj)
        FTB::VersionInfo version;
        TRY(loadVersionInfo(version, versionObj.value()))
        m.versions.append(version);
    }
    auto tagArr = Json::requireArray(obj, "tags");
    TRY(tagArr)
    for (QJsonValueRef tagRaw : tagArr.value()) {
        auto tagObj = Json::requireObject(tagRaw);
        TRY(tagObj)
        FTB::Tag tag;
        TRY(loadTag(tag, tagObj.value()))
        m.tags.append(tag);
    }
    TRY_INTO(m.updated, Json::requireInteger(obj, "updated"))
    return {};
}

static Result<> loadVersionTarget(FTB::VersionTarget& a, const QJsonObject& obj)
{
    TRY_INTO(a.id, Json::requireInteger(obj, "id"))
    TRY_INTO(a.name, Json::requireString(obj, "name"))
    TRY_INTO(a.type, Json::requireString(obj, "type"))
    TRY_INTO(a.version, Json::requireString(obj, "version"))
    TRY_INTO(a.updated, Json::requireInteger(obj, "updated"))
    return {};
}

static Result<> loadVersionFile(FTB::VersionFile& a, const QJsonObject& obj)
{
    TRY_INTO(a.id, Json::requireInteger(obj, "id"))
    TRY_INTO(a.type, Json::requireString(obj, "type"))
    TRY_INTO(a.path, Json::requireString(obj, "path"))
    TRY_INTO(a.name, Json::requireString(obj, "name"))
    TRY_INTO(a.version, Json::requireString(obj, "version"))
    a.url = obj["url"].toString();  // optional
    TRY_INTO(a.sha1, Json::requireString(obj, "sha1"))
    a.size = obj["size"].toInt();
    TRY_INTO(a.clientOnly, Json::requireBoolean(obj, "clientonly"))
    TRY_INTO(a.serverOnly, Json::requireBoolean(obj, "serveronly"))
    TRY_INTO(a.optional, Json::requireBoolean(obj, "optional"))
    TRY_INTO(a.updated, Json::requireInteger(obj, "updated"))
    auto curseforgeObj = obj["curseforge"].toObject();  // optional
    a.curseforge.project_id = curseforgeObj["project"].toInt();
    a.curseforge.file_id = curseforgeObj["file"].toInt();
    return {};
}

Result<> FTB::loadVersion(FTB::Version& m, const QJsonObject& obj)
{
    TRY_INTO(m.id, Json::requireInteger(obj, "id"))
    TRY_INTO(m.parent, Json::requireInteger(obj, "parent"))
    TRY_INTO(m.name, Json::requireString(obj, "name"))
    TRY_INTO(m.type, Json::requireString(obj, "type"))
    TRY_INTO(m.installs, Json::requireInteger(obj, "installs"))
    TRY_INTO(m.plays, Json::requireInteger(obj, "plays"))
    TRY_INTO(m.updated, Json::requireInteger(obj, "updated"))
    m.refreshed = obj["refreshed"].toInt();
    auto specs = Json::requireObject(obj, "specs");
    TRY(specs)
    TRY(loadSpecs(m.specs, specs.value()))
    auto targetArr = Json::requireArray(obj, "targets");
    TRY(targetArr)
    for (QJsonValueRef targetRaw : targetArr.value()) {
        auto versionObj = Json::requireObject(targetRaw);
        TRY(versionObj)
        FTB::VersionTarget target;
        TRY(loadVersionTarget(target, versionObj.value()))
        m.targets.append(target);
    }
    auto fileArr = Json::requireArray(obj, "files");
    TRY(fileArr)
    for (QJsonValueRef fileRaw : fileArr.value()) {
        auto fileObj = Json::requireObject(fileRaw);
        TRY(fileObj)
        FTB::VersionFile file;
        TRY(loadVersionFile(file, fileObj.value()))
        m.files.append(file);
    }
    return {};
}
