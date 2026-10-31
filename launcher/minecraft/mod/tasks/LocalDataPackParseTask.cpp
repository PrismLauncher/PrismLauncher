// SPDX-FileCopyrightText: 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
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

#include "LocalDataPackParseTask.h"

#include "FileSystem.h"
#include "Json.h"
#include "archive/ArchiveReader.h"
#include "minecraft/mod/ResourcePack.h"

#include <QCryptographicHash>
#include <utility>

namespace DataPackUtils {

bool process(DataPack* pack)
{
    switch (pack->type()) {
        case ResourceType::FOLDER:
            return DataPackUtils::processFolder(pack);
        case ResourceType::ZIPFILE:
            return DataPackUtils::processZIP(pack);
        default:
            qWarning() << "Invalid type for data pack parse task!";
            return false;
    }
}

bool processFolder(DataPack* pack)
{
    Q_ASSERT(pack->type() == ResourceType::FOLDER);

    auto mcmeta_invalid = [&pack]() {
        qWarning() << "Data pack at" << pack->fileinfo().filePath() << "does not have a valid pack.mcmeta";
        return false;  // the mcmeta is not optional
    };

    QFileInfo mcmeta_file_info(FS::PathCombine(pack->fileinfo().filePath(), "pack.mcmeta"));
    if (mcmeta_file_info.exists() && mcmeta_file_info.isFile()) {
        QFile mcmeta_file(mcmeta_file_info.filePath());
        if (!mcmeta_file.open(QIODevice::ReadOnly))
            return mcmeta_invalid();  // can't open mcmeta file

        auto data = mcmeta_file.readAll();

        bool mcmeta_result = DataPackUtils::processMCMeta(pack, std::move(data));

        mcmeta_file.close();
        if (!mcmeta_result) {
            return mcmeta_invalid();  // mcmeta invalid
        }
    } else {
        return mcmeta_invalid();  // mcmeta file isn't a valid file
    }

    return true;
}

bool processZIP(DataPack* pack)
{
    Q_ASSERT(pack->type() == ResourceType::ZIPFILE);

    MMCZip::ArchiveReader zip(pack->fileinfo().filePath());
    auto data = zip.readFile("pack.mcmeta");
    if (!data.has_value()) {
        qWarning() << "Could not read pack.mcmeta from texture pack at" << pack->fileinfo().filePath() << "-" << data.error();
        return false;
    }

    return DataPackUtils::processMCMeta(pack, std::move(data.value()));
}

std::pair<int, int> parseVersion(const QJsonValue& value)
{
    if (value.isDouble()) {
        // Single integer -> [major, 0]
        return std::make_pair(value.toInt(), 0);
    }
    std::pair<int, int> version;
    if (value.isArray()) {
        QJsonArray arr = value.toArray();
        if (arr.size() >= 1) {
            version.first = arr.at(0).toInt();
        }
        if (arr.size() >= 2) {
            version.second = arr.at(1).toInt();
        }
    }
    return version;
}

// https://minecraft.wiki/w/Data_pack#pack.mcmeta
// https://minecraft.wiki/w/Raw_JSON_text_format
// https://minecraft.wiki/w/Tutorials/Creating_a_resource_pack#Formatting_pack.mcmeta
bool processMCMeta(DataPack* pack, QByteArray&& rawData)
{
    QJsonParseError parseError;
    auto jsonDoc = Json::parseUntilGarbage(rawData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse JSON:" << parseError.errorString();
        return false;
    }

    auto packObjRes = Json::requireObject(jsonDoc.object(), "pack", {});
    if (!packObjRes) {
        qWarning() << "Could not parse data pack:" << packObjRes.error();
        return false;
    }

    const auto& packObj = packObjRes.value();
    int packFormat = 0;
    std::pair<int, int> minFormat;
    std::pair<int, int> maxFormat;
    if (packObj.contains("pack_format")) {
        packFormat = packObj.value("pack_format").toInt();
    }
    if (packObj.contains("min_format")) {
        minFormat = parseVersion(packObj.value("min_format"));
    }
    if (packObj.contains("max_format")) {
        maxFormat = parseVersion(packObj.value("max_format"));
    }
    pack->setPackFormat(packFormat, minFormat, maxFormat);
    pack->setDescription(DataPackUtils::processComponent(packObj.value("description")));
    return true;
}

QString buildStyle(const QJsonObject& obj)
{
    QStringList styles;
    if (auto color = obj["color"].toString(); !color.isEmpty()) {
        styles << QString("color: %1;").arg(color);
    }
    if (obj.contains("bold")) {
        QString weight = "normal";
        if (obj["bold"].toBool()) {
            weight = "bold";
        }
        styles << QString("font-weight: %1;").arg(weight);
    }
    if (obj.contains("italic")) {
        QString style = "normal";
        if (obj["italic"].toBool()) {
            style = "italic";
        }
        styles << QString("font-style: %1;").arg(style);
    }

    return styles.isEmpty() ? "" : QString("style=\"%1\"").arg(styles.join(" "));
}

QString processComponent(const QJsonArray& value, bool strikethrough, bool underline)
{
    QString result;
    for (auto current : value)
        result += processComponent(current, strikethrough, underline);
    return result;
}

QString processComponent(const QJsonObject& obj, bool strikethrough, bool underline)
{
    underline = obj["underlined"].toBool(underline);
    strikethrough = obj["strikethrough"].toBool(strikethrough);

    QString result = obj["text"].toString();
    if (underline) {
        result = QString("<u>%1</u>").arg(result);
    }
    if (strikethrough) {
        result = QString("<s>%1</s>").arg(result);
    }
    // the extra needs to be a array
    result += processComponent(obj["extra"].toArray(), strikethrough, underline);
    if (auto style = buildStyle(obj); !style.isEmpty()) {
        result = QString("<span %1>%2</span>").arg(style, result);
    }
    if (obj.contains("clickEvent")) {
        auto click_event = obj["clickEvent"].toObject();
        auto action = click_event["action"].toString();
        auto value = click_event["value"].toString();
        if (action == "open_url" && !value.isEmpty()) {
            result = QString("<a href=\"%1\">%2</a>").arg(value, result);
        }
    }
    return result;
}

QString processComponent(const QJsonValue& value, bool strikethrough, bool underline)
{
    if (value.isString()) {
        return value.toString();
    }
    if (value.isBool()) {
        return value.toBool() ? "true" : "false";
    }
    if (value.isDouble()) {
        return QString::number(value.toDouble());
    }
    if (value.isArray()) {
        return processComponent(value.toArray(), strikethrough, underline);
    }
    if (value.isObject()) {
        return processComponent(value.toObject(), strikethrough, underline);
    }
    qWarning() << "Invalid component type!";
    return {};
}

QPixmap processPackPNG(ResourceType type, const QString& path)
{
    auto png_invalid = [&path]() {
        qWarning() << "Data pack at" << path << "does not have a valid pack.png";
        return QPixmap();
    };

    switch (type) {
        case ResourceType::FOLDER: {
            QFileInfo image_file_info(FS::PathCombine(path, "pack.png"));
            if (image_file_info.exists() && image_file_info.isFile()) {
                QFile pack_png_file(image_file_info.filePath());
                if (!pack_png_file.open(QIODevice::ReadOnly))
                    return png_invalid();  // can't open pack.png file

                auto data = pack_png_file.readAll();

                QPixmap result;
                if (!result.loadFromData(data)) {
                    return png_invalid();
                }

                return result;
            }

            return png_invalid();  // pack.png does not exists or is not a valid file.
        }
        case ResourceType::ZIPFILE: {
            MMCZip::ArchiveReader zip(path);
            auto dataRes = zip.readFile("pack.png");
            if (!dataRes.has_value()) {
                return png_invalid();
            }

            QPixmap result;
            if (!result.loadFromData(dataRes.value())) {
                return png_invalid();  // pack.png invalid
            }

            return result;
        }
        default:
            qWarning() << "Invalid type for data pack parse task!";
            return {};
    }
}

bool validate(QFileInfo file)
{
    DataPack dp{ file };
    return DataPackUtils::process(&dp) && dp.valid();
}

bool validateResourcePack(QFileInfo file)
{
    ResourcePack rp{ file };
    return DataPackUtils::process(&rp) && rp.valid();
}

}  // namespace DataPackUtils

LocalDataPackParseTask::LocalDataPackParseTask(int token, DataPack* dp) : Task(false), m_token(token), m_data_pack(dp) {}

void LocalDataPackParseTask::executeTask()
{
    if (!DataPackUtils::process(m_data_pack)) {
        emitFailed("process failed");
        return;
    }

    emitSucceeded();
}
