// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QDirListing>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QVariant>

#include "AssetsUtils.h"
#include "FileSystem.h"
#include "net/ApiRequest.h"
#include "net/ChecksumValidator.h"

#include "Application.h"
#include "net/Request.h"
#include "update/AssetUpdateTask.h"

namespace {
QSet<QString> collectPathsFromDir(const QString& dirPath)
{
    QFileInfo dirInfo(dirPath);

    if (!dirInfo.exists()) {
        return {};
    }

    QSet<QString> out;

    for (const auto& entry : QDirListing(dirPath, QDirListing::IteratorFlag::FilesOnly | QDirListing::IteratorFlag::ResolveSymlinks |
                                                      QDirListing::IteratorFlag::Recursive)) {
        out.insert(entry.absoluteFilePath());
        qDebug() << entry.absoluteFilePath();
    }
    return out;
}
}  // namespace

namespace AssetsUtils {

/*
 * Returns true on success, with index populated
 * index is undefined otherwise
 */
bool loadAssetsIndexJson(const QString& assetsId, const QString& path, AssetsIndex& index)
{
    /*
    {
      "objects": {
        "icons/icon_16x16.png": {
          "hash": "bdf48ef6b5d0d23bbb02e17d04865216179f510a",
          "size": 3665
        },
        ...
        }
      }
    }
    */

    QFile file(path);

    // Try to open the file and fail if we can't.
    // TODO: We should probably report this error to the user.
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to read assets index file" << path << "error:" << file.errorString();
        return false;
    }
    index.id = assetsId;

    // Read the file and close it.
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

    // Fail if the JSON is invalid.
    if (parseError.error != QJsonParseError::NoError) {
        qCritical() << "Failed to parse assets index file:" << parseError.errorString() << "at offset "
                    << QString::number(parseError.offset);
        return false;
    }

    // Make sure the root is an object.
    if (!jsonDoc.isObject()) {
        qCritical() << "Invalid assets index JSON: Root should be an array.";
        return false;
    }

    QJsonObject root = jsonDoc.object();

    QJsonValue isVirtual = root.value("virtual");
    if (!isVirtual.isUndefined()) {
        index.isVirtual = isVirtual.toBool(false);
    }

    QJsonValue mapToResources = root.value("map_to_resources");
    if (!mapToResources.isUndefined()) {
        index.mapToResources = mapToResources.toBool(false);
    }

    QJsonValue objects = root.value("objects");
    QVariantMap map = objects.toVariant().toMap();

    for (QVariantMap::const_iterator iter = map.begin(); iter != map.end(); ++iter) {
        // qDebug() << iter.key();

        const auto& variant = iter.value();
        auto nestedObjects = variant.toMap();

        AssetObject object;

        for (auto nestedIter = nestedObjects.begin(); nestedIter != nestedObjects.end(); ++nestedIter) {
            // qDebug() << nested_iter.key() << nested_iter.value().toString();
            const auto& key = nestedIter.key();
            const auto& value = nestedIter.value();

            if (key == "hash") {
                object.hash = value.toString();
            } else if (key == "size") {
                object.size = value.toLongLong();
            }
        }

        index.objects.insert(iter.key(), object);
    }

    return true;
}

// FIXME: ugly code duplication
QDir getAssetsDir(const QString& assetsId, const QString& resourcesFolder)
{
    QDir assetsDir = QDir("assets/");
    QDir indexDir = QDir(FS::PathCombine(assetsDir.path(), "indexes"));
    QDir objectDir = QDir(FS::PathCombine(assetsDir.path(), "objects"));
    QDir virtualDir = QDir(FS::PathCombine(assetsDir.path(), "virtual"));

    QString indexPath = FS::PathCombine(indexDir.path(), assetsId + ".json");
    QFile indexFile(indexPath);
    QDir virtualRoot(FS::PathCombine(virtualDir.path(), assetsId));

    if (!indexFile.exists()) {
        qCritical() << "No assets index file" << indexPath << "; can't determine assets path!";
        return virtualRoot;
    }

    AssetsIndex index;
    if (!AssetsUtils::loadAssetsIndexJson(assetsId, indexPath, index)) {
        qCritical() << "Failed to load asset index file" << indexPath << "; can't determine assets path!";
        return virtualRoot;
    }

    QString targetPath;
    if (index.isVirtual) {
        return virtualRoot;
    }
    if (index.mapToResources) {
        return { resourcesFolder };
    }
    return virtualRoot;
}

// FIXME: ugly code duplication
bool reconstructAssets(const QString& assetsId, const QString& resourcesFolder)
{
    QDir assetsDir = QDir("assets/");
    QDir indexDir = QDir(FS::PathCombine(assetsDir.path(), "indexes"));
    QDir objectDir = QDir(FS::PathCombine(assetsDir.path(), "objects"));
    QDir virtualDir = QDir(FS::PathCombine(assetsDir.path(), "virtual"));

    QString indexPath = FS::PathCombine(indexDir.path(), assetsId + ".json");
    QFile indexFile(indexPath);
    QDir virtualRoot(FS::PathCombine(virtualDir.path(), assetsId));

    if (!indexFile.exists()) {
        qCritical() << "No assets index file" << indexPath << "; can't reconstruct assets!";
        return false;
    }

    qDebug() << "reconstructAssets" << assetsDir.path() << indexDir.path() << objectDir.path() << virtualDir.path() << virtualRoot.path();

    AssetsIndex index;
    if (!AssetsUtils::loadAssetsIndexJson(assetsId, indexPath, index)) {
        qCritical() << "Failed to load asset index file" << indexPath << "; can't reconstruct assets!";
        return false;
    }

    QString targetPath;
    bool removeLeftovers = false;
    if (index.isVirtual) {
        targetPath = virtualRoot.path();
        removeLeftovers = true;
        qDebug() << "Reconstructing virtual assets folder at" << targetPath;
    } else if (index.mapToResources) {
        targetPath = resourcesFolder;
        qDebug() << "Reconstructing resources folder at" << targetPath;
    }

    if (!targetPath.isNull()) {
        auto presentFiles = collectPathsFromDir(targetPath);
        for (const auto& map : index.objects.keys()) {
            const auto& assetObject = index.objects.value(map);
            QString assetPath = FS::PathCombine(targetPath, map);
            QFile target(assetPath);

            QString tlk = assetObject.hash.left(2);

            QString originalPath = FS::PathCombine(objectDir.path(), tlk, assetObject.hash);
            QFile original(originalPath);
            if (!original.exists()) {
                continue;
            }

            presentFiles.remove(assetPath);

            if (!target.exists()) {
                QFileInfo info(assetPath);
                QDir targetDir = info.dir();

                qDebug() << targetDir.path();
                FS::ensureFolderPathExists(targetDir.path());

                bool couldCopy = original.copy(assetPath);
                qDebug() << " Copying" << originalPath << "to" << assetPath << QString::number(static_cast<int>(couldCopy));
            }
        }

        // TODO: Write last used time to virtualRoot/.lastused
        if (removeLeftovers) {
            for (const auto& file : presentFiles) {
                qDebug() << "Would remove" << file;
            }
        }
    }
    return true;
}

}  // namespace AssetsUtils

Net::Request::Ptr AssetObject::getDownloadAction() const
{
    QFileInfo objectFile(getLocalPath());
    if ((!objectFile.isFile()) || (objectFile.size() != size)) {
        auto objectDL = Net::ApiRequest::makeFile(getUrl(), objectFile.filePath());
        if (!hash.isEmpty()) {
            objectDL->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, hash));
        }
        objectDL->setProgress(objectDL->getProgress(), size);
        return objectDL;
    }
    return nullptr;
}

QString AssetObject::getLocalPath() const
{
    return "assets/objects/" + getRelPath();
}

QUrl AssetObject::getUrl() const
{
    auto resourceURL = AssetUpdateTask::resourceUrl();
    return resourceURL + getRelPath();
}

QString AssetObject::getRelPath() const
{
    return hash.left(2) + "/" + hash;
}

NetJob::Ptr AssetsIndex::getDownloadJob() const
{
    auto job = makeShared<NetJob>(QObject::tr("Assets for %1").arg(id), APPLICATION->network());
    for (auto& object : objects.values()) {
        auto dl = object.getDownloadAction();
        if (dl) {
            job->addNetAction(dl);
        }
    }
    if (job->size() != 0) {
        return job;
    }
    return nullptr;
}
