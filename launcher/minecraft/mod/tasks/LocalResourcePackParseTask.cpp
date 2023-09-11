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
 */

#include "LocalResourcePackParseTask.h"

#include "FileSystem.h"
#include "Json.h"

#include <quazip/quazip.h>
#include <quazip/quazipdir.h>
#include <quazip/quazipfile.h>

#include <QCryptographicHash>

namespace ResourcePackUtils {

bool process(ResourcePack& pack, ProcessingLevel level)
{
    switch (pack.type()) {
        case ResourceType::FOLDER:
            return ResourcePackUtils::processFolder(pack, level);
        case ResourceType::ZIPFILE:
            return ResourcePackUtils::processZIP(pack, level);
        default:
            qWarning() << "Invalid type for resource pack parse task!";
            return false;
    }
}

bool processFolder(ResourcePack& pack, ProcessingLevel level)
{
    Q_ASSERT(pack.type() == ResourceType::FOLDER);

    auto mcmeta_invalid = [&pack]() {
        qWarning() << "Resource pack at" << pack.fileinfo().filePath() << "does not have a valid pack.mcmeta";
        return false;  // the mcmeta is not optional
    };

    QFileInfo mcmeta_file_info(FS::PathCombine(pack.fileinfo().filePath(), "pack.mcmeta"));
    if (mcmeta_file_info.exists() && mcmeta_file_info.isFile()) {
        QFile mcmeta_file(mcmeta_file_info.filePath());
        if (!mcmeta_file.open(QIODevice::ReadOnly))
            return mcmeta_invalid();  // can't open mcmeta file

        auto data = mcmeta_file.readAll();

        bool mcmeta_result = ResourcePackUtils::processMCMeta(pack, std::move(data));

        mcmeta_file.close();
        if (!mcmeta_result) {
            return mcmeta_invalid();  // mcmeta invalid
        }
    } else {
        return mcmeta_invalid();  // mcmeta file isn't a valid file
    }

    QFileInfo assets_dir_info(FS::PathCombine(pack.fileinfo().filePath(), "assets"));
    if (!assets_dir_info.exists() || !assets_dir_info.isDir()) {
        return false;  // assets dir does not exists or isn't valid
    }

    if (level == ProcessingLevel::BasicInfoOnly) {
        return true;  // only need basic info already checked
    }

    auto png_invalid = [&pack]() {
        qWarning() << "Resource pack at" << pack.fileinfo().filePath() << "does not have a valid pack.png";
        return true;  // the png is optional
    };

    QFileInfo image_file_info(FS::PathCombine(pack.fileinfo().filePath(), "pack.png"));
    if (image_file_info.exists() && image_file_info.isFile()) {
        QFile pack_png_file(image_file_info.filePath());
        if (!pack_png_file.open(QIODevice::ReadOnly))
            return png_invalid();  // can't open pack.png file

        auto data = pack_png_file.readAll();

        bool pack_png_result = ResourcePackUtils::processPackPNG(pack, std::move(data));

        pack_png_file.close();
        if (!pack_png_result) {
            return png_invalid();  // pack.png invalid
        }
    } else {
        return png_invalid();  // pack.png does not exists or is not a valid file.
    }

    return true;  // all tests passed
}

bool processZIP(ResourcePack& pack, ProcessingLevel level)
{
    Q_ASSERT(pack.type() == ResourceType::ZIPFILE);

    QuaZip zip(pack.fileinfo().filePath());
    if (!zip.open(QuaZip::mdUnzip))
        return false;  // can't open zip file

    QuaZipFile file(&zip);

    auto mcmeta_invalid = [&pack]() {
        qWarning() << "Resource pack at" << pack.fileinfo().filePath() << "does not have a valid pack.mcmeta";
        return false;  // the mcmeta is not optional
    };

    if (zip.setCurrentFile("pack.mcmeta")) {
        if (!file.open(QIODevice::ReadOnly)) {
            qCritical() << "Failed to open file in zip.";
            zip.close();
            return mcmeta_invalid();
        }

        auto data = file.readAll();

        bool mcmeta_result = ResourcePackUtils::processMCMeta(pack, std::move(data));

        file.close();
        if (!mcmeta_result) {
            return mcmeta_invalid();  // mcmeta invalid
        }
    } else {
        return mcmeta_invalid();  // could not set pack.mcmeta as current file.
    }

    QuaZipDir zipDir(&zip);
    if (!zipDir.exists("/assets")) {
        return false;  // assets dir does not exists at zip root
    }

    if (level == ProcessingLevel::BasicInfoOnly) {
        zip.close();
        return true;  // only need basic info already checked
    }

    auto png_invalid = [&pack]() {
        qWarning() << "Resource pack at" << pack.fileinfo().filePath() << "does not have a valid pack.png";
        return true;  // the png is optional
    };

    if (zip.setCurrentFile("pack.png")) {
        if (!file.open(QIODevice::ReadOnly)) {
            qCritical() << "Failed to open file in zip.";
            zip.close();
            return png_invalid();
        }

        auto data = file.readAll();

        bool pack_png_result = ResourcePackUtils::processPackPNG(pack, std::move(data));

        file.close();
        zip.close();
        if (!pack_png_result) {
            return png_invalid();  // pack.png invalid
        }
    } else {
        zip.close();
        return png_invalid();  // could not set pack.mcmeta as current file.
    }

    zip.close();
    return true;
}

struct TextFormat {
    QString color = "#000000";
    bool bold = false;
    bool italic = false;
    bool underlined = false;
    bool strikethrough = false;

    TextFormat() = default;
    ~TextFormat() = default;

    void clear() {
        color = "#00000";
        bold = false;
        italic = false;
        underlined = false;
        strikethrough = false;
    }
};

QString textColorToHexColor(const QString& color)
{
    static const std::unordered_map<QString, QString> str_to_hex = {
        { "black", "#000000" },
        { "dark_blue", "#0000AA" },
        { "dark_green", "#00AA00" },
        { "dark_aqua", "#00AAAA" },
        { "dark_red", "#AA0000" },
        { "dark_purple", "#AA00AA" },
        { "gold", "#FFAA00" },
        { "gray", "#AAAAAA" },
        { "dark_gray", "#555555" },
        { "blue", "#5555FF" },
        { "green", "#55FF55" },
        { "aqua", "#55FFFF" },
        { "red", "#FF5555" },
        { "light_purple", "#FF55FF" },
        { "yellow", "#FFFF55" },
        { "white", "#FFFFFF" },
    };

    auto it = str_to_hex.find(color);
    return (it != str_to_hex.end()) ? it->second : QString("#000000"); // return black if color not found
}

bool readFormat(const QJsonObject& obj, TextFormat& format) {
    auto text = obj.value("text");
    auto color = obj.value("color");
    auto bold = obj.value("bold");
    auto italic = obj.value("italic");
    auto underlined = obj.value("underlined");
    auto strikethrough = obj.value("strikethrough");
    auto extra = obj.value("extra");

    if (color.isString()) {
        format.color = textColorToHexColor(color.toString());
    }
    if (bold.isBool())
        format.bold = bold.toBool();
    if (italic.isBool())
        format.italic = italic.toBool();
    if (underlined.isBool())
        format.underlined = underlined.toBool();
    if (strikethrough.isBool())
        format.strikethrough = strikethrough.toBool();

    return true;
}

void appendBeginFormat(TextFormat& format, QString& toAppend) {
    toAppend.append("<font color=\"" + format.color + "\">");
    if (format.bold)
        toAppend.append("<b>");
    if (format.italic)
        toAppend.append("<i>");
    if (format.underlined)
        toAppend.append("<u>");
    if (format.strikethrough)
        toAppend.append("<s>");
}

void appendEndFormat(TextFormat& format, QString& toAppend) {
    toAppend.append("</font>");
    if (format.bold)
        toAppend.append("</b>");
    if (format.italic)
        toAppend.append("</i>");
    if (format.underlined)
        toAppend.append("</u>");
    if (format.strikethrough)
        toAppend.append("</s>");
}

bool processComponent(const QJsonValue& value, QString& result, TextFormat* parentFormat = nullptr);

bool processComponentList(const QJsonArray& arr, QString& result, TextFormat* parentFormat = nullptr) {

    for (const QJsonValue& val : arr) {
        processComponent(val, result, parentFormat);
    }

    return true;
}

bool processComponent(const QJsonValue& value, QString& result, TextFormat* parentFormat) {
    if (value.isString()) {
        result.append(value.toString());
    } else if (value.isObject()) {
        auto obj = value.toObject();

        TextFormat format{};
        if (parentFormat)
            format = *parentFormat;

        if (!readFormat(obj, format))
            return false;
        
        appendBeginFormat(format, result);
        
        auto text = obj.value("text");
        if (text.isString())
            result.append(text.toString());

        auto extra = obj.value("extra");
        if (extra.isArray())
            if (!processComponentList(extra.toArray(), result, &format))
                return false;

        appendEndFormat(format, result);
    }

    return true;
}

// https://minecraft.fandom.com/wiki/Tutorials/Creating_a_resource_pack#Formatting_pack.mcmeta
bool processMCMeta(ResourcePack& pack, QByteArray&& raw_data) {


    try {
        auto json_doc = QJsonDocument::fromJson(raw_data);
        auto pack_obj = Json::requireObject(json_doc.object(), "pack", {});

        pack.setPackFormat(Json::ensureInteger(pack_obj, "pack_format", 0));

        // description could either be string, or array of dictionaries
        auto desc_val = pack_obj.value("description");
        
        if (desc_val.isString())
            pack.setDescription(desc_val.toString());
        else if (desc_val.isArray()) {
            QString desc{};
            processComponentList(desc_val.toArray(), desc);
            pack.setDescription(desc);
        }

    } catch (Json::JsonException& e) {
        qWarning() << "JsonException: " << e.what() << e.cause();
        return false;
    }
    return true;
}

bool processPackPNG(const ResourcePack& pack, QByteArray&& raw_data)
{
    auto img = QImage::fromData(raw_data);
    if (!img.isNull()) {
        pack.setImage(img);
    } else {
        qWarning() << "Failed to parse pack.png.";
        return false;
    }
    return true;
}

bool processPackPNG(const ResourcePack& pack)
{
    auto png_invalid = [&pack]() {
        qWarning() << "Resource pack at" << pack.fileinfo().filePath() << "does not have a valid pack.png";
        return false;
    };

    switch (pack.type()) {
        case ResourceType::FOLDER: {
            QFileInfo image_file_info(FS::PathCombine(pack.fileinfo().filePath(), "pack.png"));
            if (image_file_info.exists() && image_file_info.isFile()) {
                QFile pack_png_file(image_file_info.filePath());
                if (!pack_png_file.open(QIODevice::ReadOnly))
                    return png_invalid();  // can't open pack.png file

                auto data = pack_png_file.readAll();

                bool pack_png_result = ResourcePackUtils::processPackPNG(pack, std::move(data));

                pack_png_file.close();
                if (!pack_png_result) {
                    return png_invalid();  // pack.png invalid
                }
            } else {
                return png_invalid();  // pack.png does not exists or is not a valid file.
            }
        }
        case ResourceType::ZIPFILE: {
            Q_ASSERT(pack.type() == ResourceType::ZIPFILE);

            QuaZip zip(pack.fileinfo().filePath());
            if (!zip.open(QuaZip::mdUnzip))
                return false;  // can't open zip file

            QuaZipFile file(&zip);
            if (zip.setCurrentFile("pack.png")) {
                if (!file.open(QIODevice::ReadOnly)) {
                    qCritical() << "Failed to open file in zip.";
                    zip.close();
                    return png_invalid();
                }

                auto data = file.readAll();

                bool pack_png_result = ResourcePackUtils::processPackPNG(pack, std::move(data));

                file.close();
                if (!pack_png_result) {
                    return png_invalid();  // pack.png invalid
                }
            } else {
                return png_invalid();  // could not set pack.mcmeta as current file.
            }
        }
        default:
            qWarning() << "Invalid type for resource pack parse task!";
            return false;
    }
}

bool validate(QFileInfo file)
{
    ResourcePack rp{ file };
    return ResourcePackUtils::process(rp, ProcessingLevel::BasicInfoOnly) && rp.valid();
}

}  // namespace ResourcePackUtils

LocalResourcePackParseTask::LocalResourcePackParseTask(int token, ResourcePack& rp)
    : Task(nullptr, false), m_token(token), m_resource_pack(rp)
{}

bool LocalResourcePackParseTask::abort()
{
    m_aborted = true;
    return true;
}

void LocalResourcePackParseTask::executeTask()
{
    if (!ResourcePackUtils::process(m_resource_pack))
        return;

    if (m_aborted)
        emitAborted();
    else
        emitSucceeded();
}
