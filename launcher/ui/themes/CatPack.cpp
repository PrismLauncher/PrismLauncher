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

#include "ui/themes/CatPack.h"
#include <QDate>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QImageReader>
#include <QRandomGenerator>
#include "FileSystem.h"
#include "Json.h"

QString BasicCatPack::path()
{
    const auto now = QDate::currentDate();
    const auto birthday = QDate(now.year(), 11, 1);
    const auto xmas = QDate(now.year(), 12, 25);
    const auto halloween = QDate(now.year(), 10, 31);

    QString cat = QString(":/backgrounds/%1").arg(m_id);
    if (std::abs(now.daysTo(xmas)) <= 4) {
        cat += "-xmas";
    } else if (std::abs(now.daysTo(halloween)) <= 4) {
        cat += "-spooky";
    } else if (std::abs(now.daysTo(birthday)) <= 12) {
        cat += "-bday";
    }
    return cat;
}

JsonCatPack::PartialDate partialDate(QJsonObject date)
{
    auto month = Json::ensureInteger(date, "month", 1);
    if (month > 12)
        month = 12;
    else if (month <= 0)
        month = 1;
    auto day = Json::ensureInteger(date, "day", 1);
    if (day > 31)
        day = 31;
    else if (day <= 0)
        day = 1;
    return { month, day };
};

JsonCatPack::JsonCatPack(QFileInfo& manifestInfo) : BasicCatPack(manifestInfo.dir().dirName())
{
    QString path = manifestInfo.path();
    auto doc = Json::requireDocument(manifestInfo.absoluteFilePath(), "CatPack JSON file");
    const auto root = doc.object();
    m_name = Json::requireString(root, "name", "Catpack name");
    m_default_path = FS::PathCombine(path, Json::requireString(root, "default", "Default Cat"));
    auto variants = Json::ensureArray(root, "variants", QJsonArray(), "Catpack Variants");
    for (auto v : variants) {
        auto variant = Json::ensureObject(v, QJsonObject(), "Cat variant");
        m_variants << Variant{ FS::PathCombine(path, Json::requireString(variant, "path", "Variant path")),
                               partialDate(Json::requireObject(variant, "startTime", "Variant startTime")),
                               partialDate(Json::requireObject(variant, "endTime", "Variant endTime")) };
    }
}

QDate ensureDay(int year, int month, int day)
{
    QDate date(year, month, 1);
    if (day > date.daysInMonth())
        day = date.daysInMonth();
    return QDate(year, month, day);
}

QString JsonCatPack::path()
{
    return path(QDate::currentDate());
}

QString JsonCatPack::path(QDate now)
{
    for (auto var : m_variants) {
        QDate startDate = ensureDay(now.year(), var.startTime.month, var.startTime.day);
        QDate endDate = ensureDay(now.year(), var.endTime.month, var.endTime.day);
        if (startDate > endDate) {  // it's spans over multiple years
            if (endDate < now)      // end date is in the past so jump one year into the future for endDate
                endDate = endDate.addYears(1);
            else  // end date is in the future so jump one year into the past for startDate
                startDate = startDate.addYears(-1);
        }

        if (startDate <= now && now <= endDate)
            return var.path;
    }
    auto dInfo = QFileInfo(m_default_path);
    if (!dInfo.isDir())
        return m_default_path;

    QStringList supportedImageFormats;
    for (auto format : QImageReader::supportedImageFormats()) {
        supportedImageFormats.append("*." + format);
    }

    auto files = QDir(m_default_path).entryInfoList(supportedImageFormats, QDir::Files, QDir::Name);
    if (files.length() == 0)
        return "";
    auto idx = (now.dayOfYear() - 1) % files.length();
    auto isRandom = dInfo.fileName().compare("random", Qt::CaseInsensitive) == 0;
    if (isRandom)
        idx = QRandomGenerator::global()->bounded(0, files.length());
    return files[idx].absoluteFilePath();
}
