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
#include <QRegularExpression>

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

struct TextFormatter {
    // left is value, right is if the value was explicitly written
    QPair<QString, bool> color = { "#000000", false };
    QPair<bool, bool> bold = { false, false };
    QPair<bool, bool> italic = { false, false };
    QPair<bool, bool> underlined = { false, false };
    QPair<bool, bool> strikethrough = { false, false };
    QPair<bool, bool> is_linked = { false, false };
    QPair<QString, bool> link_url = { "", false };

    void setColor(const QString& new_color, bool written) { color = { new_color, written }; }
    void setBold(bool new_bold, bool written) { bold = { new_bold, written }; }
    void setItalic(bool new_italic, bool written) { italic = { new_italic, written }; }
    void setUnderlined(bool new_underlined, bool written) { underlined = { new_underlined, written }; }
    void setStrikethrough(bool new_strikethrough, bool written) { strikethrough = { new_strikethrough, written }; }
    void setIsLinked(bool new_is_linked, bool written) { is_linked = { new_is_linked, written }; }
    void setLinkURL(const QString& new_url, bool written) { link_url = { new_url, written }; }

    void overrideFrom(const TextFormatter& child)
    {
        if (child.color.second)
            color.first = child.color.first;
        if (child.bold.second)
            bold.first = child.bold.first;
        if (child.italic.second)
            italic.first = child.italic.first;
        if (child.underlined.second)
            underlined.first = child.underlined.first;
        if (child.strikethrough.second)
            strikethrough.first = child.strikethrough.first;
        if (child.is_linked.second)
            is_linked.first = child.is_linked.first;
        if (child.link_url.second)
            link_url.first = child.link_url.first;
    }

    QString format(QString text)
    {
        if (text.isEmpty())
            return QString();

        QString result;

        if (color.first != "#000000")
            result.append("<font color=\"" + color.first + "\">");
        if (bold.first)
            result.append("<b>");
        if (italic.first)
            result.append("<i>");
        if (underlined.first)
            result.append("<u>");
        if (strikethrough.first)
            result.append("<s>");
        if (is_linked.first)
            result.append("<a href=\"" + link_url.first + "\">");

        result.append(text);

        if (is_linked.first)
            result.append("</a>");
        if (strikethrough.first)
            result.append("</s>");
        if (underlined.first)
            result.append("</u>");
        if (italic.first)
            result.append("</i>");
        if (bold.first)
            result.append("</b>");
        if (color.first != "#000000")
            result.append("</font>");

        return result;
    }

    bool readFormat(const QJsonObject& obj)
    {
        setColor(Json::ensureString(obj, "color", "#000000"), obj.contains("color"));
        setBold(Json::ensureBoolean(obj, "bold", false), obj.contains("bold"));
        setItalic(Json::ensureBoolean(obj, "italic", false), obj.contains("italic"));
        setUnderlined(Json::ensureBoolean(obj, "underlined", false), obj.contains("underlined"));
        setStrikethrough(Json::ensureBoolean(obj, "strikethrough", false), obj.contains("strikethrough"));

        auto click_event = Json::ensureObject(obj, "clickEvent");
        setIsLinked(Json::ensureBoolean(click_event, "open_url", false), click_event.contains("open_url"));
        setLinkURL(Json::ensureString(click_event, "value"), click_event.contains("value"));

        return true;
    }
};

bool processComponent(const QJsonValue& value, QString& result, const TextFormatter* parentFormat)
{
    TextFormatter formatter;

    if (parentFormat)
        formatter = *parentFormat;

    if (value.isString()) {
        result.append(formatter.format(value.toString()));
    } else if (value.isBool()) {
        result.append(formatter.format(value.toBool() ? "true" : "false"));
    } else if (value.isDouble()) {
        result.append(formatter.format(QString::number(value.toDouble())));
    } else if (value.isObject()) {
        auto obj = value.toObject();

        if (not formatter.readFormat(obj))
            return false;

        // override the parent format with our new one
        TextFormatter mixed;
        if (parentFormat)
            mixed = *parentFormat;

        mixed.overrideFrom(formatter);

        result.append(mixed.format(Json::ensureString(obj, "text")));

        // process any 'extra' children with this format
        auto extra = obj.value("extra");
        if (not extra.isUndefined())
            return processComponent(extra, result, &mixed);

    } else if (value.isArray()) {
        auto array = value.toArray();

        for (const QJsonValue& current : array) {
            if (not processComponent(current, result, parentFormat)) {
                return false;
            }
        }
    } else {
        qWarning() << "Invalid component type!";
        return false;
    }

    return true;
}

// https://minecraft.fandom.com/wiki/Tutorials/Creating_a_resource_pack#Formatting_pack.mcmeta
// https://minecraft.fandom.com/wiki/Raw_JSON_text_format#Plain_Text
bool processMCMeta(ResourcePack& pack, QByteArray&& raw_data)
{
    try {
        auto json_doc = QJsonDocument::fromJson(raw_data);
        auto pack_obj = Json::requireObject(json_doc.object(), "pack", {});

        pack.setPackFormat(Json::ensureInteger(pack_obj, "pack_format", 0));

        auto desc_val = pack_obj.value("description");
        QString desc{};
        if (not processComponent(desc_val, desc))
            return false;

        qInfo() << desc;
        pack.setDescription(desc);

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
