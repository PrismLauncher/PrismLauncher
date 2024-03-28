// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

#include "MojangVersionFormat.h"
#include "MojangDownloadInfo.h"
#include "OneSixVersionFormat.h"

#include "Json.h"
using namespace Json;
#include <BuildConfig.h>
#include "ParseUtils.h"

static const int CURRENT_MINIMUM_LAUNCHER_VERSION = 18;

static MojangAssetIndexInfo::Ptr assetIndexFromJson(const QJsonObject& obj);
static MojangDownloadInfo::Ptr downloadInfoFromJson(const QJsonObject& obj);
static MojangLibraryDownloadInfo::Ptr libDownloadInfoFromJson(const QJsonObject& libObj);
static QJsonObject assetIndexToJson(MojangAssetIndexInfo::Ptr assetidxinfo);
static QJsonObject libDownloadInfoToJson(MojangLibraryDownloadInfo::Ptr libinfo);
static QJsonObject downloadInfoToJson(MojangDownloadInfo::Ptr info);

namespace Bits {
static void readString(const QJsonObject& root, const QString& key, QString& variable)
{
    if (root.contains(key)) {
        variable = requireString(root.value(key));
    }
}

static void readDownloadInfo(MojangDownloadInfo::Ptr out, const QJsonObject& obj)
{
    // optional, not used
    readString(obj, "path", out->path);
    // required!
    out->sha1 = requireString(obj, "sha1");
    out->url = requireString(obj, "url");
    out->size = requireInteger(obj, "size");
}

static void readAssetIndex(MojangAssetIndexInfo::Ptr out, const QJsonObject& obj)
{
    out->totalSize = requireInteger(obj, "totalSize");
    out->id = requireString(obj, "id");
    // out->known = true;
}
}  // namespace Bits

MojangDownloadInfo::Ptr downloadInfoFromJson(const QJsonObject& obj)
{
    auto out = std::make_shared<MojangDownloadInfo>();
    Bits::readDownloadInfo(out, obj);
    return out;
}

MojangAssetIndexInfo::Ptr assetIndexFromJson(const QJsonObject& obj)
{
    auto out = std::make_shared<MojangAssetIndexInfo>();
    Bits::readDownloadInfo(out, obj);
    Bits::readAssetIndex(out, obj);
    return out;
}

QJsonObject downloadInfoToJson(MojangDownloadInfo::Ptr info)
{
    QJsonObject out;
    if (!info->path.isNull()) {
        out.insert("path", info->path);
    }
    out.insert("sha1", info->sha1);
    out.insert("size", info->size);
    out.insert("url", info->url);
    return out;
}

MojangLibraryDownloadInfo::Ptr libDownloadInfoFromJson(const QJsonObject& libObj)
{
    auto out = std::make_shared<MojangLibraryDownloadInfo>();
    auto dlObj = requireObject(libObj.value("downloads"));
    if (dlObj.contains("artifact")) {
        out->artifact = downloadInfoFromJson(requireObject(dlObj, "artifact"));
    }
    if (dlObj.contains("classifiers")) {
        auto classifiersObj = requireObject(dlObj, "classifiers");
        for (auto iter = classifiersObj.begin(); iter != classifiersObj.end(); iter++) {
            auto classifier = iter.key();
            auto classifierObj = requireObject(iter.value());
            out->classifiers[classifier] = downloadInfoFromJson(classifierObj);
        }
    }
    return out;
}

QJsonObject libDownloadInfoToJson(MojangLibraryDownloadInfo::Ptr libinfo)
{
    QJsonObject out;
    if (libinfo->artifact) {
        out.insert("artifact", downloadInfoToJson(libinfo->artifact));
    }
    if (!libinfo->classifiers.isEmpty()) {
        QJsonObject classifiersOut;
        for (auto iter = libinfo->classifiers.begin(); iter != libinfo->classifiers.end(); iter++) {
            classifiersOut.insert(iter.key(), downloadInfoToJson(iter.value()));
        }
        out.insert("classifiers", classifiersOut);
    }
    return out;
}

QJsonObject assetIndexToJson(MojangAssetIndexInfo::Ptr info)
{
    QJsonObject out;
    if (!info->path.isNull()) {
        out.insert("path", info->path);
    }
    out.insert("sha1", info->sha1);
    out.insert("size", info->size);
    out.insert("url", info->url);
    out.insert("totalSize", info->totalSize);
    out.insert("id", info->id);
    return out;
}

void MojangVersionFormat::readVersionProperties(const QJsonObject& in, VersionFile* out)
{
    Bits::readString(in, "id", out->minecraftVersion);
    Bits::readString(in, "mainClass", out->mainClass);
    Bits::readString(in, "minecraftArguments", out->minecraftArguments);
    Bits::readString(in, "type", out->type);

    Bits::readString(in, "assets", out->assets);
    if (in.contains("assetIndex")) {
        out->mojangAssetIndex = assetIndexFromJson(requireObject(in, "assetIndex"));
    } else if (!out->assets.isNull()) {
        out->mojangAssetIndex = std::make_shared<MojangAssetIndexInfo>(out->assets);
    }

    out->releaseTime = timeFromS3Time(in.value("releaseTime").toString(""));
    out->updateTime = timeFromS3Time(in.value("time").toString(""));

    if (in.contains("minimumLauncherVersion")) {
        out->minimumLauncherVersion = requireInteger(in.value("minimumLauncherVersion"));
        if (out->minimumLauncherVersion > CURRENT_MINIMUM_LAUNCHER_VERSION) {
            out->addProblem(ProblemSeverity::Warning, QObject::tr("The 'minimumLauncherVersion' value of this version (%1) is higher than "
                                                                  "supported by %3 (%2). It might not work properly!")
                                                          .arg(out->minimumLauncherVersion)
                                                          .arg(CURRENT_MINIMUM_LAUNCHER_VERSION)
                                                          .arg(BuildConfig.LAUNCHER_DISPLAYNAME));
        }
    }

    if (in.contains("compatibleJavaMajors")) {
        for (auto compatible : requireArray(in.value("compatibleJavaMajors"))) {
            out->compatibleJavaMajors.append(requireInteger(compatible));
        }
    }
    if (in.contains("compatibleJavaName")) {
        out->compatibleJavaName = requireString(in.value("compatibleJavaName"));
    }

    if (in.contains("downloads")) {
        auto downloadsObj = requireObject(in, "downloads");
        for (auto iter = downloadsObj.begin(); iter != downloadsObj.end(); iter++) {
            auto classifier = iter.key();
            auto classifierObj = requireObject(iter.value());
            out->mojangDownloads[classifier] = downloadInfoFromJson(classifierObj);
        }
    }
}

VersionFilePtr MojangVersionFormat::versionFileFromJson(const QJsonDocument& doc, const QString& filename)
{
    VersionFilePtr out(new VersionFile());
    if (doc.isEmpty() || doc.isNull()) {
        throw JSONValidationError(filename + " is empty or null");
    }
    if (!doc.isObject()) {
        throw JSONValidationError(filename + " is not an object");
    }

    QJsonObject root = doc.object();

    readVersionProperties(root, out.get());

    out->name = "Minecraft";
    out->uid = "net.minecraft";
    out->version = out->minecraftVersion;
    // out->filename = filename;

    if (root.contains("libraries")) {
        for (auto libVal : requireArray(root.value("libraries"))) {
            auto libObj = requireObject(libVal);

            auto lib = MojangVersionFormat::libraryFromJson(*out, libObj, filename);
            out->libraries.append(lib);
        }
    }
    return out;
}

void MojangVersionFormat::writeVersionProperties(const VersionFile* in, QJsonObject& out)
{
    writeString(out, "id", in->minecraftVersion);
    writeString(out, "mainClass", in->mainClass);
    writeString(out, "minecraftArguments", in->minecraftArguments);
    writeString(out, "type", in->type);
    if (!in->releaseTime.isNull()) {
        writeString(out, "releaseTime", timeToS3Time(in->releaseTime));
    }
    if (!in->updateTime.isNull()) {
        writeString(out, "time", timeToS3Time(in->updateTime));
    }
    if (in->minimumLauncherVersion != -1) {
        out.insert("minimumLauncherVersion", in->minimumLauncherVersion);
    }
    writeString(out, "assets", in->assets);
    if (in->mojangAssetIndex && in->mojangAssetIndex->known) {
        out.insert("assetIndex", assetIndexToJson(in->mojangAssetIndex));
    }
    if (!in->mojangDownloads.isEmpty()) {
        QJsonObject downloadsOut;
        for (auto iter = in->mojangDownloads.begin(); iter != in->mojangDownloads.end(); iter++) {
            downloadsOut.insert(iter.key(), downloadInfoToJson(iter.value()));
        }
        out.insert("downloads", downloadsOut);
    }
    if (!in->compatibleJavaMajors.isEmpty()) {
        QJsonArray compatibleJavaMajorsOut;
        for (auto compatibleJavaMajor : in->compatibleJavaMajors) {
            compatibleJavaMajorsOut.append(compatibleJavaMajor);
        }
        out.insert("compatibleJavaMajors", compatibleJavaMajorsOut);
    }
    if (!in->compatibleJavaName.isEmpty()) {
        writeString(out, "compatibleJavaName", in->compatibleJavaName);
    }
}

QJsonDocument MojangVersionFormat::versionFileToJson(const VersionFilePtr& patch)
{
    QJsonObject root;
    writeVersionProperties(patch.get(), root);
    if (!patch->libraries.isEmpty()) {
        QJsonArray array;
        for (auto value : patch->libraries) {
            array.append(MojangVersionFormat::libraryToJson(value.get()));
        }
        root.insert("libraries", array);
    }

    // write the contents to a json document.
    {
        QJsonDocument out;
        out.setObject(root);
        return out;
    }
}

LibraryPtr MojangVersionFormat::libraryFromJson(ProblemContainer& problems, const QJsonObject& libObj, const QString& filename)
{
    LibraryPtr out(new Library());
    if (!libObj.contains("name")) {
        throw JSONValidationError(filename + "contains a library that doesn't have a 'name' field");
    }
    auto rawName = libObj.value("name").toString();
    out->m_name = rawName;
    if (!out->m_name.valid()) {
        problems.addProblem(ProblemSeverity::Error, QObject::tr("Library %1 name is broken and cannot be processed.").arg(rawName));
    }

    Bits::readString(libObj, "url", out->m_repositoryURL);
    if (libObj.contains("extract")) {
        out->m_hasExcludes = true;
        auto extractObj = requireObject(libObj.value("extract"));
        for (auto excludeVal : requireArray(extractObj.value("exclude"))) {
            out->m_extractExcludes.append(requireString(excludeVal));
        }
    }
    if (libObj.contains("natives")) {
        QJsonObject nativesObj = requireObject(libObj.value("natives"));
        for (auto it = nativesObj.begin(); it != nativesObj.end(); ++it) {
            if (!it.value().isString()) {
                qWarning() << filename << "contains an invalid native (skipping)";
            }
            // FIXME: Skip unknown platforms
            out->m_nativeClassifiers[it.key()] = it.value().toString();
        }
    }
    if (libObj.contains("rules")) {
        out->applyRules = true;
        out->m_rules = rulesFromJsonV4(libObj);
    }
    if (libObj.contains("downloads")) {
        out->m_mojangDownloads = libDownloadInfoFromJson(libObj);
    }
    return out;
}

QJsonObject MojangVersionFormat::libraryToJson(Library* library)
{
    QJsonObject libRoot;
    libRoot.insert("name", library->m_name.serialize());
    if (!library->m_repositoryURL.isEmpty()) {
        libRoot.insert("url", library->m_repositoryURL);
    }
    if (library->isNative()) {
        QJsonObject nativeList;
        auto iter = library->m_nativeClassifiers.begin();
        while (iter != library->m_nativeClassifiers.end()) {
            nativeList.insert(iter.key(), iter.value());
            iter++;
        }
        libRoot.insert("natives", nativeList);
        if (!library->m_extractExcludes.isEmpty()) {
            QJsonArray excludes;
            QJsonObject extract;
            for (auto exclude : library->m_extractExcludes) {
                excludes.append(exclude);
            }
            extract.insert("exclude", excludes);
            libRoot.insert("extract", extract);
        }
    }
    if (!library->m_rules.isEmpty()) {
        QJsonArray allRules;
        for (auto& rule : library->m_rules) {
            QJsonObject ruleObj = rule->toJson();
            allRules.append(ruleObj);
        }
        libRoot.insert("rules", allRules);
    }
    if (library->m_mojangDownloads) {
        auto downloadsObj = libDownloadInfoToJson(library->m_mojangDownloads);
        libRoot.insert("downloads", downloadsObj);
    }
    return libRoot;
}
