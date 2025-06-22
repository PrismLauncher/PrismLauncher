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
    virtual QString descriptor() override { return version.toString(); }

    virtual QString name() override { return m_name; }

    virtual QString typeString() const override { return vendor; }

    virtual bool operator<(BaseVersion& a) override;
    virtual bool operator>(BaseVersion& a) override;
    bool operator<(const Metadata& rhs);
    bool operator==(const Metadata& rhs);
    bool operator>(const Metadata& rhs);

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

DownloadType parseDownloadType(QString javaDownload);
QString downloadTypeToString(DownloadType javaDownload);
MetadataPtr parseJavaMeta(const QJsonObject& libObj);

}  // namespace Java