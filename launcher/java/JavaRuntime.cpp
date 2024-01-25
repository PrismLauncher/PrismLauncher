#include "java/JavaRuntime.h"

#include <memory>

#include "Json.h"
#include "java/JavaVersion.h"
#include "minecraft/ParseUtils.h"

namespace JavaRuntime {

DownloadType parseDownloadType(QString javaDownload)
{
    if (javaDownload == "manifest")
        return DownloadType::Manifest;
    // if (javaDownload == "archive")
    return DownloadType::Archive;
}
QString downloadTypeToString(DownloadType javaDownload)
{
    switch (javaDownload) {
        case DownloadType::Manifest:
            return "manifest";
        case DownloadType::Archive:
            return "archive";
    }
}
MetaPtr parseJavaMeta(const QJsonObject& in)
{
    auto meta = std::make_shared<Meta>();

    meta->name = Json::ensureString(in, "name", "");
    meta->vendor = Json::ensureString(in, "vendor", "");
    meta->url = Json::ensureString(in, "url", "");
    meta->releaseTime = timeFromS3Time(Json::ensureString(in, "releaseTime", ""));
    meta->recommended = Json::ensureBoolean(in, "recommended", false);
    meta->downloadType = parseDownloadType(Json::ensureString(in, "downloadType", ""));
    meta->packageType = Json::ensureString(in, "packageType", "");

    if (in.contains("checksum")) {
        auto obj = Json::requireObject(in, "checksum");
        meta->checksumHash = Json::ensureString(obj, "hash", "");
        meta->checksumType = Json::ensureString(obj, "type", "");
    }

    if (in.contains("version")) {
        auto obj = Json::requireObject(in, "checksum");
        auto name = Json::ensureString(obj, "name", "");
        auto major = Json::ensureInteger(obj, "major", 0);
        auto minor = Json::ensureInteger(obj, "minor", 0);
        auto security = Json::ensureInteger(obj, "security", 0);
        auto build = Json::ensureInteger(obj, "build", 0);
        meta->version = JavaVersion(major, minor, security, build, name);
    }
    return meta;
}
}  // namespace JavaRuntime
