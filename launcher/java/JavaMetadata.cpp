// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023-2024 Trial97 <alexandru.tripon97@gmail.com>
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
 */

#include "java/JavaMetadata.h"

#include <memory>

#include "Json.h"
#include "StringUtils.h"
#include "java/JavaVersion.h"
#include "minecraft/ParseUtils.h"

namespace Java {

DownloadType parseDownloadType(QString javaDownload)
{
    if (javaDownload == "manifest")
        return DownloadType::Manifest;
    else if (javaDownload == "archive")
        return DownloadType::Archive;
    else
        return DownloadType::Unknown;
}
QString downloadTypeToString(DownloadType javaDownload)
{
    switch (javaDownload) {
        case DownloadType::Manifest:
            return "manifest";
        case DownloadType::Archive:
            return "archive";
        case DownloadType::Unknown:
            break;
    }
    return "unknown";
}
MetadataPtr parseJavaMeta(const QJsonObject& in)
{
    auto meta = std::make_shared<Metadata>();

    meta->m_name = Json::ensureString(in, "name", "");
    meta->vendor = Json::ensureString(in, "vendor", "");
    meta->url = Json::ensureString(in, "url", "");
    meta->releaseTime = timeFromS3Time(Json::ensureString(in, "releaseTime", ""));
    meta->downloadType = parseDownloadType(Json::ensureString(in, "downloadType", ""));
    meta->packageType = Json::ensureString(in, "packageType", "");
    meta->runtimeOS = Json::ensureString(in, "runtimeOS", "unknown");

    if (in.contains("checksum")) {
        auto obj = Json::requireObject(in, "checksum");
        meta->checksumHash = Json::ensureString(obj, "hash", "");
        meta->checksumType = Json::ensureString(obj, "type", "");
    }

    if (in.contains("version")) {
        auto obj = Json::requireObject(in, "version");
        auto name = Json::ensureString(obj, "name", "");
        auto major = Json::ensureInteger(obj, "major", 0);
        auto minor = Json::ensureInteger(obj, "minor", 0);
        auto security = Json::ensureInteger(obj, "security", 0);
        auto build = Json::ensureInteger(obj, "build", 0);
        meta->version = JavaVersion(major, minor, security, build, name);
    }
    return meta;
}

bool Metadata::operator<(const Metadata& rhs)
{
    auto id = version;
    if (id < rhs.version) {
        return true;
    }
    if (id > rhs.version) {
        return false;
    }
    auto date = releaseTime;
    if (date < rhs.releaseTime) {
        return true;
    }
    if (date > rhs.releaseTime) {
        return false;
    }
    return StringUtils::naturalCompare(m_name, rhs.m_name, Qt::CaseInsensitive) < 0;
}

bool Metadata::operator==(const Metadata& rhs)
{
    return version == rhs.version && m_name == rhs.m_name;
}

bool Metadata::operator>(const Metadata& rhs)
{
    return (!operator<(rhs)) && (!operator==(rhs));
}

bool Metadata::operator<(BaseVersion& a)
{
    try {
        return operator<(dynamic_cast<Metadata&>(a));
    } catch (const std::bad_cast& e) {
        return BaseVersion::operator<(a);
    }
}

bool Metadata::operator>(BaseVersion& a)
{
    try {
        return operator>(dynamic_cast<Metadata&>(a));
    } catch (const std::bad_cast& e) {
        return BaseVersion::operator>(a);
    }
}

}  // namespace Java
