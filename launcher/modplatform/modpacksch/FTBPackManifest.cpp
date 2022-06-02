/*
 * Copyright 2020 Jamie Mansfield <jmansfield@cadixdev.org>
 * Copyright 2020-2021 Petr Mrazek <peterix@gmail.com>
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

#include "FTBPackManifest.h"

#include "Json.h"

static void loadSpecs(ModpacksCH::Specs & s, QJsonObject & obj)
{
    s.id = Json::requireInteger(obj, "id");
    s.minimum = Json::requireInteger(obj, "minimum");
    s.recommended = Json::requireInteger(obj, "recommended");
}

static void loadTag(ModpacksCH::Tag & t, QJsonObject & obj)
{
    t.id = Json::requireInteger(obj, "id");
    t.name = Json::requireString(obj, "name");
}

static void loadArt(ModpacksCH::Art & a, QJsonObject & obj)
{
    a.id = Json::requireInteger(obj, "id");
    a.url = Json::requireString(obj, "url");
    a.type = Json::requireString(obj, "type");
    a.width = Json::requireInteger(obj, "width");
    a.height = Json::requireInteger(obj, "height");
    a.compressed = Json::requireBoolean(obj, "compressed");
    a.sha1 = Json::requireString(obj, "sha1");
    a.size = Json::requireInteger(obj, "size");
    a.updated = Json::requireInteger(obj, "updated");
}

static void loadAuthor(ModpacksCH::Author & a, QJsonObject & obj)
{
    a.id = Json::requireInteger(obj, "id");
    a.name = Json::requireString(obj, "name");
    a.type = Json::requireString(obj, "type");
    a.website = Json::requireString(obj, "website");
    a.updated = Json::requireInteger(obj, "updated");
}

static void loadVersionInfo(ModpacksCH::VersionInfo & v, QJsonObject & obj)
{
    v.id = Json::requireInteger(obj, "id");
    v.name = Json::requireString(obj, "name");
    v.type = Json::requireString(obj, "type");
    v.updated = Json::requireInteger(obj, "updated");
    auto specs = Json::requireObject(obj, "specs");
    loadSpecs(v.specs, specs);
}

void ModpacksCH::loadModpack(ModpacksCH::Modpack & m, QJsonObject & obj)
{
    m.id = Json::requireInteger(obj, "id");
    m.name = Json::requireString(obj, "name");
    m.synopsis = Json::requireString(obj, "synopsis");
    m.description = Json::requireString(obj, "description");
    m.type = Json::requireString(obj, "type");
    m.featured = Json::requireBoolean(obj, "featured");
    m.installs = Json::requireInteger(obj, "installs");
    m.plays = Json::requireInteger(obj, "plays");
    m.updated = Json::requireInteger(obj, "updated");
    m.refreshed = Json::requireInteger(obj, "refreshed");
    auto artArr = Json::requireArray(obj, "art");
    for (QJsonValueRef artRaw : artArr)
    {
        auto artObj = Json::requireObject(artRaw);
        ModpacksCH::Art art;
        loadArt(art, artObj);
        m.art.append(art);
    }
    auto authorArr = Json::requireArray(obj, "authors");
    for (QJsonValueRef authorRaw : authorArr)
    {
        auto authorObj = Json::requireObject(authorRaw);
        ModpacksCH::Author author;
        loadAuthor(author, authorObj);
        m.authors.append(author);
    }
    auto versionArr = Json::requireArray(obj, "versions");
    for (QJsonValueRef versionRaw : versionArr)
    {
        auto versionObj = Json::requireObject(versionRaw);
        ModpacksCH::VersionInfo version;
        loadVersionInfo(version, versionObj);
        m.versions.append(version);
    }
    auto tagArr = Json::requireArray(obj, "tags");
    for (QJsonValueRef tagRaw : tagArr)
    {
        auto tagObj = Json::requireObject(tagRaw);
        ModpacksCH::Tag tag;
        loadTag(tag, tagObj);
        m.tags.append(tag);
    }
    m.updated = Json::requireInteger(obj, "updated");
}

static void loadVersionTarget(ModpacksCH::VersionTarget & a, QJsonObject & obj)
{
    a.id = Json::requireInteger(obj, "id");
    a.name = Json::requireString(obj, "name");
    a.type = Json::requireString(obj, "type");
    a.version = Json::requireString(obj, "version");
    a.updated = Json::requireInteger(obj, "updated");
}

static void loadVersionFile(ModpacksCH::VersionFile & a, QJsonObject & obj)
{
    a.id = Json::requireInteger(obj, "id");
    a.type = Json::requireString(obj, "type");
    a.path = Json::requireString(obj, "path");
    a.name = Json::requireString(obj, "name");
    a.version = Json::requireString(obj, "version");
    a.url = Json::ensureString(obj, "url");  // optional
    a.sha1 = Json::requireString(obj, "sha1");
    a.size = Json::requireInteger(obj, "size");
    a.clientOnly = Json::requireBoolean(obj, "clientonly");
    a.serverOnly = Json::requireBoolean(obj, "serveronly");
    a.optional = Json::requireBoolean(obj, "optional");
    a.updated = Json::requireInteger(obj, "updated");
    auto curseforgeObj = Json::ensureObject(obj, "curseforge");  // optional
    a.curseforge.project = Json::ensureInteger(curseforgeObj, "project");
    a.curseforge.file = Json::ensureInteger(curseforgeObj, "file");
}

void ModpacksCH::loadVersion(ModpacksCH::Version & m, QJsonObject & obj)
{
    m.id = Json::requireInteger(obj, "id");
    m.parent = Json::requireInteger(obj, "parent");
    m.name = Json::requireString(obj, "name");
    m.type = Json::requireString(obj, "type");
    m.installs = Json::requireInteger(obj, "installs");
    m.plays = Json::requireInteger(obj, "plays");
    m.updated = Json::requireInteger(obj, "updated");
    m.refreshed = Json::requireInteger(obj, "refreshed");
    auto specs = Json::requireObject(obj, "specs");
    loadSpecs(m.specs, specs);
    auto targetArr = Json::requireArray(obj, "targets");
    for (QJsonValueRef targetRaw : targetArr)
    {
        auto versionObj = Json::requireObject(targetRaw);
        ModpacksCH::VersionTarget target;
        loadVersionTarget(target, versionObj);
        m.targets.append(target);
    }
    auto fileArr = Json::requireArray(obj, "files");
    for (QJsonValueRef fileRaw : fileArr)
    {
        auto fileObj = Json::requireObject(fileRaw);
        ModpacksCH::VersionFile file;
        loadVersionFile(file, fileObj);
        m.files.append(file);
    }
}

//static void loadVersionChangelog(ModpacksCH::VersionChangelog & m, QJsonObject & obj)
//{
//    m.content = Json::requireString(obj, "content");
//    m.updated = Json::requireInteger(obj, "updated");
//}
