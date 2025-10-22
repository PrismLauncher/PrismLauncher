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

#include "modplatform/import_ftb/PackHelpers.h"

#include <QIcon>
#include <QImageReader>
#include <QString>
#include <QVariant>

#include "FileSystem.h"
#include "Json.h"

namespace FTBImportAPP {

QIcon loadFTBIcon(const QString& imagePath)
{
    // Map of type byte to image type string
    static const QHash<char, QByteArray> imageTypeMap = { { 0x00, "png" }, { 0x01, "jpg" }, { 0x02, "gif" }, { 0x03, "webp" } };
    QFile file(imagePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return QIcon();
    }
    char type;
    if (!file.getChar(&type)) {
        qDebug() << "Missing FTB image type header at" << imagePath;
        return QIcon();
    }
    if (!imageTypeMap.contains(type)) {
        qDebug().nospace().noquote() << "Don't recognize FTB image type 0x" << QString::number(type, 16);
        return QIcon();
    }

    auto imageType = imageTypeMap[type];
    // Extract actual image data beyond the first byte
    QImageReader reader(&file, imageType);
    auto pixmap = QPixmap::fromImageReader(&reader);
    if (pixmap.isNull()) {
        qDebug() << "The FTB image at" << imagePath << "is not valid";
        return QIcon();
    }
    return QIcon(pixmap);
}

Modpack parseDirectory(QString path)
{
    Modpack modpack{ path };
    auto instanceFile = QFileInfo(FS::PathCombine(path, "instance.json"));
    if (!instanceFile.exists() || !instanceFile.isFile())
        return {};
    try {
        auto doc = Json::requireDocument(instanceFile.absoluteFilePath(), "FTB_APP instance JSON file");
        const auto root = doc.object();
        modpack.uuid = Json::requireString(root, "uuid", "uuid");
        modpack.id = Json::requireInteger(root, "id", "id");
        modpack.versionId = Json::requireInteger(root, "versionId", "versionId");
        modpack.name = Json::requireString(root, "name", "name");
        modpack.version = Json::requireString(root, "version", "version");
        modpack.mcVersion = Json::requireString(root, "mcVersion", "mcVersion");
        modpack.jvmArgs = Json::ensureVariant(root, "jvmArgs", {}, "jvmArgs");
        modpack.totalPlayTime = Json::requireInteger(root, "totalPlayTime", "totalPlayTime");
    } catch (const Exception& e) {
        qDebug() << "Couldn't load ftb instance json: " << e.cause();
        return {};
    }

    auto versionsFile = QFileInfo(FS::PathCombine(path, "version.json"));
    if (!versionsFile.exists() || !versionsFile.isFile()) {
        versionsFile = QFileInfo(FS::PathCombine(path, ".ftbapp", "version.json"));
    }
    if (!versionsFile.exists() || !versionsFile.isFile()) {
        return {};
    }
    try {
        auto doc = Json::requireDocument(versionsFile.absoluteFilePath(), "FTB_APP version JSON file");
        const auto root = doc.object();
        auto targets = Json::requireArray(root, "targets", "targets");

        for (auto target : targets) {
            auto obj = Json::requireObject(target, "target");
            auto name = Json::requireString(obj, "name", "name");
            auto version = Json::requireString(obj, "version", "version");
            if (name == "neoforge") {
                modpack.loaderType = ModPlatform::NeoForge;
                modpack.version = version;
                break;
            } else if (name == "forge") {
                modpack.loaderType = ModPlatform::Forge;
                modpack.version = version;
                break;
            } else if (name == "fabric") {
                modpack.loaderType = ModPlatform::Fabric;
                modpack.version = version;
                break;
            } else if (name == "quilt") {
                modpack.loaderType = ModPlatform::Quilt;
                modpack.version = version;
                break;
            }
        }
    } catch (const Exception& e) {
        qDebug() << "Couldn't load ftb version json: " << e.cause();
        return {};
    }
    auto iconFile = QFileInfo(FS::PathCombine(path, "folder.jpg"));
    if (iconFile.exists() && iconFile.isFile()) {
        modpack.icon = QIcon(iconFile.absoluteFilePath());
    } else {  // the logo is a file that the first bit denotes the image tipe followed by the actual image data
        modpack.icon = loadFTBIcon(FS::PathCombine(path, ".ftbapp", "logo"));
    }
    return modpack;
}

}  // namespace FTBImportAPP
