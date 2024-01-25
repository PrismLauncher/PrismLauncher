// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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
#pragma once

#include <QDateTime>
#include <QString>

#include <memory>

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