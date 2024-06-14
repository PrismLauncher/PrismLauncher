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
#include "ExportToModList.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace ExportToModList {
QString toHTML(QList<Mod*> mods, OptionalData extraData)
{
    QStringList lines;
    for (auto mod : mods) {
        auto meta = mod->metadata();
        auto modName = mod->name().toHtmlEscaped();
        if (extraData & Url) {
            auto url = mod->metaurl().toHtmlEscaped();
            if (!url.isEmpty())
                modName = QString("<a href=\"%1\">%2</a>").arg(url, modName);
        }
        auto line = modName;
        if (extraData & Version) {
            auto ver = mod->version();
            if (ver.isEmpty() && meta != nullptr)
                ver = meta->version().toString();
            if (!ver.isEmpty())
                line += QString(" [%1]").arg(ver.toHtmlEscaped());
        }
        if (extraData & Authors && !mod->authors().isEmpty())
            line += " by " + mod->authors().join(", ").toHtmlEscaped();
        if (extraData & FileName)
            line += QString(" (%1)").arg(mod->fileinfo().fileName().toHtmlEscaped());

        lines.append(QString("<li>%1</li>").arg(line));
    }
    return QString("<html><body><ul>\n\t%1\n</ul></body></html>").arg(lines.join("\n\t"));
}

QString toMarkdownEscaped(QString src)
{
    for (auto ch : "\\`*_{}[]<>()#+-.!|")
        src.replace(ch, QString("\\%1").arg(ch));
    return src;
}

QString toMarkdown(QList<Mod*> mods, OptionalData extraData)
{
    QStringList lines;

    for (auto mod : mods) {
        auto meta = mod->metadata();
        auto modName = toMarkdownEscaped(mod->name());
        if (extraData & Url) {
            auto url = mod->metaurl();
            if (!url.isEmpty())
                modName = QString("[%1](%2)").arg(modName, url);
        }
        auto line = modName;
        if (extraData & Version) {
            auto ver = toMarkdownEscaped(mod->version());
            if (ver.isEmpty() && meta != nullptr)
                ver = toMarkdownEscaped(meta->version().toString());
            if (!ver.isEmpty())
                line += QString(" [%1]").arg(ver);
        }
        if (extraData & Authors && !mod->authors().isEmpty())
            line += " by " + toMarkdownEscaped(mod->authors().join(", "));
        if (extraData & FileName)
            line += QString(" (%1)").arg(toMarkdownEscaped(mod->fileinfo().fileName()));
        lines << "- " + line;
    }
    return lines.join("\n");
}

QString toPlainTXT(QList<Mod*> mods, OptionalData extraData)
{
    QStringList lines;
    for (auto mod : mods) {
        auto meta = mod->metadata();
        auto modName = mod->name();

        auto line = modName;
        if (extraData & Url) {
            auto url = mod->metaurl();
            if (!url.isEmpty())
                line += QString(" (%1)").arg(url);
        }
        if (extraData & Version) {
            auto ver = mod->version();
            if (ver.isEmpty() && meta != nullptr)
                ver = meta->version().toString();
            if (!ver.isEmpty())
                line += QString(" [%1]").arg(ver);
        }
        if (extraData & Authors && !mod->authors().isEmpty())
            line += " by " + mod->authors().join(", ");
        if (extraData & FileName)
            line += QString(" (%1)").arg(mod->fileinfo().fileName());
        lines << line;
    }
    return lines.join("\n");
}

QString toJSON(QList<Mod*> mods, OptionalData extraData)
{
    QJsonArray lines;
    for (auto mod : mods) {
        auto meta = mod->metadata();
        auto modName = mod->name();
        QJsonObject line;
        line["name"] = modName;
        if (extraData & Url) {
            auto url = mod->metaurl();
            if (!url.isEmpty())
                line["url"] = url;
        }
        if (extraData & Version) {
            auto ver = mod->version();
            if (ver.isEmpty() && meta != nullptr)
                ver = meta->version().toString();
            if (!ver.isEmpty())
                line["version"] = ver;
        }
        if (extraData & Authors && !mod->authors().isEmpty())
            line["authors"] = QJsonArray::fromStringList(mod->authors());
        if (extraData & FileName)
            line["filename"] = mod->fileinfo().fileName();
        lines << line;
    }
    QJsonDocument doc;
    doc.setArray(lines);
    return doc.toJson();
}

QString toCSV(QList<Mod*> mods, OptionalData extraData)
{
    QStringList lines;
    for (auto mod : mods) {
        QStringList data;
        auto meta = mod->metadata();
        auto modName = mod->name();

        data << modName;
        if (extraData & Url)
            data << mod->metaurl();
        if (extraData & Version) {
            auto ver = mod->version();
            if (ver.isEmpty() && meta != nullptr)
                ver = meta->version().toString();
            data << ver;
        }
        if (extraData & Authors) {
            QString authors;
            if (mod->authors().length() == 1)
                authors = mod->authors().back();
            else if (mod->authors().length() > 1)
                authors = QString("\"%1\"").arg(mod->authors().join(","));
            data << authors;
        }
        if (extraData & FileName)
            data << mod->fileinfo().fileName();
        lines << data.join(",");
    }
    return lines.join("\n");
}

QString exportToModList(QList<Mod*> mods, Formats format, OptionalData extraData)
{
    switch (format) {
        case HTML:
            return toHTML(mods, extraData);
        case MARKDOWN:
            return toMarkdown(mods, extraData);
        case PLAINTXT:
            return toPlainTXT(mods, extraData);
        case JSON:
            return toJSON(mods, extraData);
        case CSV:
            return toCSV(mods, extraData);
        default: {
            return QString("unknown format:%1").arg(format);
        }
    }
}

QString exportToModList(QList<Mod*> mods, QString lineTemplate)
{
    QStringList lines;
    for (auto mod : mods) {
        auto meta = mod->metadata();
        auto modName = mod->name();
        auto url = mod->metaurl();
        auto ver = mod->version();
        if (ver.isEmpty() && meta != nullptr)
            ver = meta->version().toString();
        auto authors = mod->authors().join(", ");
        auto filename = mod->fileinfo().fileName();
        lines << QString(lineTemplate)
                     .replace("{name}", modName)
                     .replace("{url}", url)
                     .replace("{version}", ver)
                     .replace("{authors}", authors)
                     .replace("{filename}", filename);
    }
    return lines.join("\n");
}
}  // namespace ExportToModList