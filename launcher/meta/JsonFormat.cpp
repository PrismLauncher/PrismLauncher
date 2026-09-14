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
Result<std::shared_ptr<Meta::Index>> parseIndexInternal(const QJsonObject& obj)
{
    const auto objects = requireIsArrayOf<QJsonObject>(obj, "packages");
    TRY(objects)

    QList<Meta::VersionList::Ptr> lists;
    lists.reserve(objects->size());

    for (const auto& entry : objects.value()) {
        auto uid = requireString(entry, "uid");
        TRY(uid)

        auto list = std::make_shared<Meta::VersionList>(uid.value());
        list->setName(entry["name"].toString());
        list->setSha256(entry["sha256"].toString());

        lists.push_back(list);
    }
    return std::make_shared<Meta::Index>(lists);
}

// Version
Result<Meta::Version::Ptr> parseCommonVersion(const QString& uid, const QJsonObject& obj)
{
    auto versionRsp = requireString(obj, "version");
    TRY(versionRsp)
    auto releaseTime = requireString(obj, "releaseTime");
    TRY(releaseTime)

    auto version = std::make_shared<Meta::Version>(uid, versionRsp.value());
    version->setTime(QDateTime::fromString(releaseTime.value(), Qt::ISODate).toMSecsSinceEpoch() / 1000);
    version->setType(obj["type"].toString());
    version->setRecommended(obj["recommended"].toBool());
    version->setVolatile(obj["volatile"].toBool());

    Meta::RequireSet reqs;
    Meta::RequireSet conflicts;
    TRY(parseRequires(obj, &reqs, "requires"))
    TRY(parseRequires(obj, &conflicts, "conflicts"))
    version->setRequires(reqs, conflicts);

    if (auto sha256 = obj["sha256"].toString(); !sha256.isEmpty()) {
        version->setSha256(sha256);
    }
    return version;
}

Result<Meta::Version::Ptr> parseVersionInternal(const QJsonObject& obj)
{
    auto uid = requireString(obj, "uid");
    TRY(uid)
    auto versionRsp = parseCommonVersion(uid.value(), obj);
    TRY(versionRsp)
    auto version = versionRsp.value();

    auto data = OneSixVersionFormat::versionFileFromJson(QJsonDocument(obj), QString("%1/%2.json").arg(version->uid(), version->version()),
                                                         obj.contains("order"));
    TRY(data)
    version->setData(data.value());
    return version;
}

// Version list / package
Result<Meta::VersionList::Ptr> parseVersionListInternal(const QJsonObject& obj)
{
    const auto uid = requireString(obj, "uid");
    TRY(uid)

    const auto versionsRaw = requireIsArrayOf<QJsonObject>(obj, "versions");
    TRY(versionsRaw)

    QList<Meta::Version::Ptr> versions;
    versions.reserve(versionsRaw->size());
    for (const auto& v : versionsRaw.value()) {
        auto versionRsp = parseCommonVersion(uid.value(), v);
        TRY(versionRsp)

        const auto& version = versionRsp.value();
        version->setProvidesRecommendations();
        versions.push_back(version);
    }

    auto list = std::make_shared<Meta::VersionList>(uid.value());
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

Result<> parseIndex(const QJsonObject& obj, Index* ptr)
{
    const auto version = parseFormatVersion(obj);
    TRY(version)
    const auto index = parseIndexInternal(obj);
    TRY(index)
    ptr->merge(index.value());
    return {};
}

Result<> parseVersionList(const QJsonObject& obj, VersionList* ptr)
{
    const auto version = parseFormatVersion(obj);
    TRY(version)
    auto list = parseVersionListInternal(obj);
    TRY(list)
    ptr->merge(list.value());
    return {};
}

Result<> parseVersion(const QJsonObject& obj, Version* ptr)
{
    const auto version = parseFormatVersion(obj);
    TRY(version)
    auto ver = parseVersionInternal(obj);
    TRY(ver)
    ptr->merge(ver.value());
    return {};
}

/*
[
{"uid":"foo", "equals":"version"}
]
*/
Result<> parseRequires(const QJsonObject& obj, RequireSet* ptr, const char* keyName)
{
    if (obj.contains(keyName)) {
        auto reqArray = requireArray(obj, keyName);
        TRY(reqArray)
        for (const auto iter : reqArray.value()) {
            auto reqObj = requireObject(iter);
            TRY(reqObj)
            auto reqObject = reqObj.value();
            auto uid = requireString(reqObject, "uid");
            TRY(uid)
            auto equals = reqObject["equals"].toString();
            auto suggests = reqObject["suggests"].toString();
            ptr->insert({ .uid = uid.value(), .equalsVersion = equals, .suggests = suggests });
        }
    }
    return {};
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
