#pragma once

#include <QDateTime>
#include <QString>

#include "java/JavaVersion.h"

namespace JavaRuntime {

enum class DownloadType { Manifest, Archive };

struct Meta {
    QString name;
    QString vendor;
    QString url;
    QDateTime releaseTime;
    QString checksumType;
    QString checksumHash;
    bool recommended;
    DownloadType downloadType;
    QString packageType;
    JavaVersion version;
};
using MetaPtr = std::shared_ptr<Meta>;

DownloadType parseDownloadType(QString javaDownload);
QString downloadTypeToString(DownloadType javaDownload);
MetaPtr parseJavaMeta(const QJsonObject& libObj);

}  // namespace JavaRuntime