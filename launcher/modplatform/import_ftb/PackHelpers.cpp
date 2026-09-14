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
#include <expected>

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

Result<Modpack> parseDirectory(const QString& path)
{
    Modpack modpack{ .path = path };
    auto instanceFile = QFileInfo(FS::PathCombine(path, "instance.json"));
    if (!instanceFile.exists() || !instanceFile.isFile()) {
        return std::unexpected("Couldn't find ftb instance json");
    }
    auto doc = Json::requireDocument(instanceFile.absoluteFilePath(), "FTB_APP instance JSON file");
    TRY(doc)
    const auto root = doc->object();
    TRY_INTO(modpack.uuid, Json::requireString(root, "uuid", "uuid"))
    TRY_INTO(modpack.id, Json::requireInteger(root, "id", "id"))
    TRY_INTO(modpack.versionId, Json::requireInteger(root, "versionId", "versionId"))
    TRY_INTO(modpack.name, Json::requireString(root, "name", "name"))
    TRY_INTO(modpack.version, Json::requireString(root, "version", "version"))
    TRY_INTO(modpack.mcVersion, Json::requireString(root, "mcVersion", "mcVersion"))
    modpack.jvmArgs = root["jvmArgs"].toVariant();
    TRY_INTO(modpack.totalPlayTime, Json::requireInteger(root, "totalPlayTime", "totalPlayTime"))

    auto modLoader = Json::requireString(root, "modLoader", "modLoader");
    TRY(modLoader)
    if (!modLoader->isEmpty()) {
        const auto parts = modLoader->split('-', Qt::KeepEmptyParts);
        if (parts.size() >= 2) {
            const auto loader = parts.first().toLower();
            modpack.loaderVersion = parts.at(1).trimmed();
            if (loader == "neoforge") {
                modpack.loaderType = ModPlatform::NeoForge;
            } else if (loader == "forge") {
                modpack.loaderType = ModPlatform::Forge;
            } else if (loader == "fabric") {
                modpack.loaderType = ModPlatform::Fabric;
            } else if (loader == "quilt") {
                modpack.loaderType = ModPlatform::Quilt;
            }
        }
    }
    if (!modpack.loaderType.has_value()) {
        if (auto res = legacyInstanceParsing(path, &modpack.loaderType, &modpack.loaderVersion); !res) {
            qDebug() << res.error();
        }
    }

    auto iconFile = QFileInfo(FS::PathCombine(path, "folder.jpg"));
    if (iconFile.exists() && iconFile.isFile()) {
        modpack.icon = QIcon(iconFile.absoluteFilePath());
    } else {  // the logo is a file that the first bit denotes the image tipe followed by the actual image data
        modpack.icon = loadFTBIcon(FS::PathCombine(path, ".ftbapp", "logo"));
    }
    return modpack;
}

Result<> legacyInstanceParsing(const QString& path, std::optional<ModPlatform::ModLoaderType>* loaderType, QString* loaderVersion)
{
    auto versionsFile = QFileInfo(FS::PathCombine(path, ".ftbapp", "version.json"));
    if (!versionsFile.exists() || !versionsFile.isFile()) {
        versionsFile = QFileInfo(FS::PathCombine(path, "version.json"));
    }
    if (!versionsFile.exists() || !versionsFile.isFile()) {
        return std::unexpected("Couldn't find ftb version json");
    }
    auto targets = Json::requireDocument(versionsFile.absoluteFilePath(), "FTB_APP version JSON file").and_then([](const auto& v) {
        const auto root = v.object();
        return Json::requireArray(root, "targets", "targets");
    });
    TRY(targets)

    for (auto target : targets.value()) {
        auto obj = Json::requireObject(target, "target");
        TRY(obj)
        QString name;
        QString version;
        TRY_INTO(name, Json::requireString(obj.value(), "name", "name"))
        TRY_INTO(version, Json::requireString(obj.value(), "version", "version"))
        if (name == "neoforge") {
            *loaderType = ModPlatform::NeoForge;
            *loaderVersion = version;
            break;
        }
        if (name == "forge") {
            *loaderType = ModPlatform::Forge;
            *loaderVersion = version;
            break;
        }
        if (name == "fabric") {
            *loaderType = ModPlatform::Fabric;
            *loaderVersion = version;
            break;
        }
        if (name == "quilt") {
            *loaderType = ModPlatform::Quilt;
            *loaderVersion = version;
            break;
        }
    }
    return {};
}
}  // namespace FTBImportAPP
