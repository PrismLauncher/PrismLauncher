/* Copyright 2015-2021 MultiMC Contributors
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

#include "JsonFormat.h"

#include <algorithm>

// FIXME: remove this from here... somehow
#include "Json.h"
#include "minecraft/OneSixVersionFormat.h"

#include "Index.h"
#include "Version.h"
#include "VersionList.h"

using namespace Json;

namespace {
// Index
std::shared_ptr<Meta::Index> parseIndexInternal(const QJsonObject& obj)
{
    const auto objects = requireIsArrayOf<QJsonObject>(obj, "packages");
    QList<Meta::VersionList::Ptr> lists;
    lists.reserve(objects.size());
    std::ranges::transform(objects, std::back_inserter(lists), [](const QJsonObject& obj) {
        auto list = std::make_shared<Meta::VersionList>(requireString(obj, "uid"));
        list->setName(obj["name"].toString());
        list->setSha256(obj["sha256"].toString());
        return list;
    });
    return std::make_shared<Meta::Index>(lists);
}

// Version
Meta::Version::Ptr parseCommonVersion(const QString& uid, const QJsonObject& obj)
{
    auto version = std::make_shared<Meta::Version>(uid, requireString(obj, "version"));
    version->setTime(QDateTime::fromString(requireString(obj, "releaseTime"), Qt::ISODate).toMSecsSinceEpoch() / 1000);
    version->setType(obj["type"].toString());
    version->setRecommended(obj["recommended"].toBool());
    version->setVolatile(obj["volatile"].toBool());
    Meta::RequireSet reqs;
    Meta::RequireSet conflicts;
    parseRequires(obj, &reqs, "requires");
    parseRequires(obj, &conflicts, "conflicts");
    version->setRequires(reqs, conflicts);
    if (auto sha256 = obj["sha256"].toString(); !sha256.isEmpty()) {
        version->setSha256(sha256);
    }
    return version;
}

Meta::Version::Ptr parseVersionInternal(const QJsonObject& obj)
{
    auto version = parseCommonVersion(requireString(obj, "uid"), obj);

    version->setData(OneSixVersionFormat::versionFileFromJson(
        QJsonDocument(obj), QString("%1/%2.json").arg(version->uid(), version->version()), obj.contains("order")));
    return version;
}

// Version list / package
Meta::VersionList::Ptr parseVersionListInternal(const QJsonObject& obj)
{
    const auto uid = requireString(obj, "uid");

    const auto versionsRaw = requireIsArrayOf<QJsonObject>(obj, "versions");
    QList<Meta::Version::Ptr> versions;
    versions.reserve(versionsRaw.size());
    std::ranges::transform(versionsRaw, std::back_inserter(versions), [uid](const QJsonObject& vObj) {
        auto version = parseCommonVersion(uid, vObj);
        version->setProvidesRecommendations();
        return version;
    });

    auto list = std::make_shared<Meta::VersionList>(uid);
    list->setName(obj["name"].toString());
    list->setVersions(versions);
    return list;
}

}  // namespace

namespace Meta {

Result<int> parseFormatVersion(const QJsonObject& obj, bool required)
{
    if (!obj.contains("formatVersion")) {
        if (required) {
            return std::unexpected("format version is missing");
        }
        return 1;
    }
    if (!obj.value("formatVersion").isDouble()) {
        return std::unexpected("format version is not a number");
    }
    switch (obj.value("formatVersion").toInt()) {
        case 0:
        case 1:
            return 1;
        default:
            return std::unexpected("format version is not supported");
    }
}

void serializeFormatVersion(QJsonObject& obj, int version)
{
    obj.insert("formatVersion", version);
}

Result<void> parseIndex(const QJsonObject& obj, Index* ptr)
{
    const auto version = parseFormatVersion(obj);
    if (!version) {
        return std::unexpected(version.error());
    }
    ptr->merge(parseIndexInternal(obj));
    return {};
}

Result<void> parseVersionList(const QJsonObject& obj, VersionList* ptr)
{
    const auto version = parseFormatVersion(obj);
    if (!version) {
        return std::unexpected(version.error());
    }
    ptr->merge(parseVersionListInternal(obj));
    return {};
}

Result<void> parseVersion(const QJsonObject& obj, Version* ptr)
{
    const auto version = parseFormatVersion(obj);
    if (!version) {
        return std::unexpected(version.error());
    }
    ptr->merge(parseVersionInternal(obj));
    return {};
}

/*
[
{"uid":"foo", "equals":"version"}
]
*/
void parseRequires(const QJsonObject& obj, RequireSet* ptr, const char* keyName)
{
    if (obj.contains(keyName)) {
        auto reqArray = requireArray(obj, keyName);
        auto iter = reqArray.begin();
        while (iter != reqArray.end()) {
            auto reqObject = requireObject(*iter);
            auto uid = requireString(reqObject, "uid");
            auto equals = reqObject["equals"].toString();
            auto suggests = reqObject["suggests"].toString();
            ptr->insert({ .uid = uid, .equalsVersion = equals, .suggests = suggests });
            iter++;
        }
    }
}
void serializeRequires(QJsonObject& obj, RequireSet* ptr, const char* keyName)
{
    if (!ptr || ptr->empty()) {
        return;
    }
    QJsonArray arrOut;
    for (const auto& iter : *ptr) {
        QJsonObject reqOut;
        reqOut.insert("uid", iter.uid);
        if (!iter.equalsVersion.isEmpty()) {
            reqOut.insert("equals", iter.equalsVersion);
        }
        if (!iter.suggests.isEmpty()) {
            reqOut.insert("suggests", iter.suggests);
        }
        arrOut.append(reqOut);
    }
    obj.insert(keyName, arrOut);
}

}  // namespace Meta
