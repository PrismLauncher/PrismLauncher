/* Copyright 2013-2021 MultiMC Contributors
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

#pragma once

#include <QMap>
#include <QString>
#include "net/NetJob.h"
#include "net/NetRequest.h"

struct AssetObject {
    QString getRelPath();
    QUrl getUrl();
    QString getLocalPath();
    Net::NetRequest::Ptr getDownloadAction();

    QString hash;
    qint64 size;
};

struct AssetsIndex {
    NetJob::Ptr getDownloadJob();

    QString id;
    QMap<QString, AssetObject> objects;
    bool isVirtual = false;
    bool mapToResources = false;
};

/// FIXME: this is absolutely horrendous. REDO!!!!
namespace AssetsUtils {
bool loadAssetsIndexJson(const QString& id, const QString& file, AssetsIndex& index);

QDir getAssetsDir(const QString& assetsId, const QString& resourcesFolder);

/// Reconstruct a virtual assets folder for the given assets ID and return the folder
bool reconstructAssets(QString assetsId, QString resourcesFolder);
}  // namespace AssetsUtils
