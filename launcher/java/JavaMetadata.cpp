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
Result<MetadataPtr> parseJavaMeta(const QJsonObject& in)
{
    auto meta = std::make_shared<Metadata>();

    meta->m_name = in["name"].toString("");
    meta->vendor = in["vendor"].toString("");
    meta->url = in["url"].toString("");
    meta->releaseTime = timeFromS3Time(in["releaseTime"].toString(""));
    meta->downloadType = parseDownloadType(in["downloadType"].toString(""));
    meta->packageType = in["packageType"].toString("");
    meta->runtimeOS = in["runtimeOS"].toString("unknown");

    if (in.contains("checksum")) {
        auto checksum = Json::requireObject(in, "checksum");
        TRY(checksum)
        meta->checksumHash = checksum.value()["hash"].toString("");
        meta->checksumType = checksum.value()["type"].toString("");
    }

    if (in.contains("version")) {
        auto version = Json::requireObject(in, "version");
        TRY(version)
        auto name = version.value()["name"].toString("");
        auto major = version.value()["major"].toInteger();
        auto minor = version.value()["minor"].toInteger();
        auto security = version.value()["security"].toInteger();
        auto build = version.value()["build"].toInteger();
        meta->version = JavaVersion(major, minor, security, build, name);
    }
    return meta;
}

bool Metadata::operator<(const Metadata& rhs) const
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

bool Metadata::operator==(const Metadata& rhs) const
{
    return version == rhs.version && m_name == rhs.m_name;
}

bool Metadata::operator>(const Metadata& rhs) const
{
    return (!operator<(rhs)) && (!operator==(rhs));
}

bool Metadata::operator<(BaseVersion& a) const
{
    if (auto* metadata = dynamic_cast<Metadata*>(&a)) {
        return operator<(*metadata);
    }
    return BaseVersion::operator<(a);
}

bool Metadata::operator>(BaseVersion& a) const
{
    if (auto* metadata = dynamic_cast<Metadata*>(&a)) {
        return operator>(*metadata);
    }
    return BaseVersion::operator>(a);
}

}  // namespace Java
