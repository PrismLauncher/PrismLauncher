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
    TRY_INTO(const auto& objects, requireIsArrayOf<QJsonObject>(obj, "packages"))

    QList<Meta::VersionList::Ptr> lists;
    lists.reserve(objects.size());

    for (const auto& entry : objects) {
        TRY_INTO(const auto& uid, requireString(entry, "uid"))

        auto list = std::make_shared<Meta::VersionList>(uid);
        list->setName(entry["name"].toString());
        list->setSha256(entry["sha256"].toString());

        lists.push_back(list);
    }
    return std::make_shared<Meta::Index>(lists);
}

// Version
Result<Meta::Version::Ptr> parseCommonVersion(const QString& uid, const QJsonObject& obj)
{
    TRY_INTO(const auto& versionRes, requireString(obj, "version"))
    TRY_INTO(const auto& releaseTime, requireString(obj, "releaseTime"))

    auto version = std::make_shared<Meta::Version>(uid, versionRes);
    version->setTime(QDateTime::fromString(releaseTime, Qt::ISODate).toMSecsSinceEpoch() / 1000);
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
    TRY_INTO(const auto& uid, requireString(obj, "uid"))
    TRY_INTO(const auto& version, parseCommonVersion(uid, obj))

    TRY_INTO(const auto& data,
             OneSixVersionFormat::versionFileFromJson(QJsonDocument(obj), QString("%1/%2.json").arg(version->uid(), version->version()),
                                                      obj.contains("order")))
    version->setData(data);
    return version;
}

// Version list / package
Result<Meta::VersionList::Ptr> parseVersionListInternal(const QJsonObject& obj)
{
    TRY_INTO(const auto& uid, requireString(obj, "uid"))
    TRY_INTO(const auto& versionsRaw, requireIsArrayOf<QJsonObject>(obj, "versions"))

    QList<Meta::Version::Ptr> versions;
    versions.reserve(versionsRaw.size());
    for (const auto& v : versionsRaw) {
        TRY_INTO(const auto& version, parseCommonVersion(uid, v))

        version->setProvidesRecommendations();
        versions.push_back(version);
    }

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

Result<> parseIndex(const QJsonObject& obj, Index* ptr)
{
    TRY(parseFormatVersion(obj))
    TRY_INTO(const auto& index, parseIndexInternal(obj))
    ptr->merge(index);
    return {};
}

Result<> parseVersionList(const QJsonObject& obj, VersionList* ptr)
{
    TRY(parseFormatVersion(obj))
    TRY_INTO(const auto& list, parseVersionListInternal(obj))
    ptr->merge(list);
    return {};
}

Result<> parseVersion(const QJsonObject& obj, Version* ptr)
{
    TRY(parseFormatVersion(obj))
    TRY_INTO(const auto& ver, parseVersionInternal(obj))
    ptr->merge(ver);
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
        TRY_INTO(const auto& reqArray, requireArray(obj, keyName))
        for (const auto iter : reqArray) {
            TRY_INTO(const auto& reqObject, requireObject(iter))
            TRY_INTO(const auto& uid, requireString(reqObject, "uid"))
            auto equals = reqObject["equals"].toString();
            auto suggests = reqObject["suggests"].toString();
            ptr->insert({ .uid = uid, .equalsVersion = equals, .suggests = suggests });
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
