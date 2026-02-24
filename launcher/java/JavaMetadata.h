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
#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QString>

#include <memory>

#include "BaseVersion.h"
#include "java/JavaVersion.h"

namespace Java {

enum class DownloadType { Manifest, Archive, Unknown };

class Metadata : public BaseVersion {
   public:
    QString descriptor() const override { return version.toString(); }

    QString name() const override { return m_name; }

    QString typeString() const override { return vendor; }

    bool operator<(BaseVersion& a) const override;
    bool operator>(BaseVersion& a) const override;
    bool operator<(const Metadata& rhs) const;
    bool operator==(const Metadata& rhs) const;
    bool operator>(const Metadata& rhs) const;

    QString m_name;
    QString vendor;
    QString url;
    QDateTime releaseTime;
    QString checksumType;
    QString checksumHash;
    DownloadType downloadType;
    QString packageType;
    JavaVersion version;
    QString runtimeOS;
};
using MetadataPtr = std::shared_ptr<Metadata>;

DownloadType parseDownloadType(const QString& javaDownload);
QString downloadTypeToString(DownloadType javaDownload);
MetadataPtr parseJavaMeta(const QJsonObject& libObj);

}  // namespace Java
