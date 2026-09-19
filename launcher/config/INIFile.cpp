// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 flowln <flowlnlnln@gmail.com>
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

#include "INIFile.h"

#include <AssertHelpers.h>
#include <FileSystem.h>

#include <QDebug>
#include <QFile>
#include <QStringList>
#include <QTemporaryFile>
#include <QTextStream>

#include <QSettings>
#include <utility>
#include "Json.h"

INIFile::INIFile() = default;

Result<> INIFile::saveFile(const QString& fileName)
{
    if (!contains("ConfigVersion")) {
        insert("ConfigVersion", "1.4");
    }
    QSettings settingsObj{ fileName, QSettings::Format::IniFormat };
    settingsObj.setFallbacksEnabled(false);
    settingsObj.clear();

    for (Iterator iter = begin(); iter != end(); iter++) {
        settingsObj.setValue(iter.key(), iter.value());
    }

    settingsObj.sync();

    if (auto status = settingsObj.status(); status != QSettings::Status::NoError) {
        if (status == QSettings::Status::AccessError) {
            return std::unexpected("Failed to write INI file to " + fileName);
        }
        // NOTE: FormatError can be set by the read that happens when constructing QSettings whether you like it or not
        // since we're ignoring the existing values anyway it doesn't make much sense to care about this
        if (status != QSettings::Status::FormatError) {
            return std::unexpected("Unknown error occurred while saving INI file to " + fileName);
        }
    }

    return {};
}

namespace {

QString unescape(const QString& orig)
{
    QString out;
    QChar prev = QChar::Null;
    for (auto c : orig) {
        if (prev == '\\') {
            if (c == 'n') {
                out += '\n';
            } else if (c == 't') {
                out += '\t';
            } else if (c == '#') {
                out += '#';
            } else {
                out += c;
            }
            prev = QChar::Null;
        } else {
            if (c == '\\') {
                prev = c;
                continue;
            }
            out += c;
            prev = QChar::Null;
        }
    }
    return out;
}

QString unquote(QString str)
{
    if ((str.contains(QChar(';')) || str.contains(QChar('=')) || str.contains(QChar(','))) && str.endsWith("\"") && str.startsWith("\"")) {
        str = str.removeFirst().removeLast();
    }
    return str;
}

bool parseOldFileFormat(QIODevice& device, QSettings::SettingsMap& map)
{
    QTextStream in(device.readAll());
    QStringList lines = in.readAll().split('\n');
    for (int i = 0; i < lines.count(); i++) {
        QString& lineRaw = lines[i];
        // Ignore comments.
        qsizetype commentIndex = 0;
        QString line = lineRaw;
        // Search for comments until no more escaped # are available
        while ((commentIndex = line.indexOf('#', commentIndex + 1)) != -1) {
            if (commentIndex > 0 && line.at(commentIndex - 1) == '\\') {
                continue;
            }
            line = line.left(lineRaw.indexOf('#')).trimmed();
        }

        auto eqPos = line.indexOf('=');
        if (eqPos == -1) {
            continue;
        }
        QString key = line.left(eqPos).trimmed();
        QString valueStr = line.right(line.length() - eqPos - 1).trimmed();

        valueStr = unquote(unescape(valueStr));

        QVariant value(valueStr);
        map.insert(key, value);
    }

    return true;
}

QVariant migrateQByteArrayToBase64(const QString& key, QVariant value)
{
    static const QStringList s_otherByteArrays = { "MainWindowState",       "MainWindowGeometry", "ConsoleWindowState",
                                                   "ConsoleWindowGeometry", "PagedGeometry",      "NewInstanceGeometry",
                                                   "ModDownloadGeometry",   "RPDownloadGeometry", "TPDownloadGeometry",
                                                   "ShaderDownloadGeometry" };
    if (key.startsWith("UI/") && key.endsWith("_Page/Columns")) {
        return QString::fromUtf8(value.toByteArray().toBase64());
    }
    if (s_otherByteArrays.contains(key)) {
        return QString::fromUtf8(value.toByteArray());
    }
    if (key == "linkedInstances") {
        return Json::fromStringList(value.toStringList());
    }
    if (key == "Env") {
        return Json::fromMap(value.toMap());
    }
    return value;
}

void migrateUIKeys(INIFile& file)
{
    auto remap = [&file](const QString& src, const QString& dst) {
        if (file.contains(src)) {
            file[dst] = file.take(src);
        }
    };
    remap("MainWindowGeometry", "UIGeometry/MainWindow");
    remap("ConsoleWindowGeometry", "UIGeometry/ConsoleWindow");
    remap("PageDialog", "UIGeometry/PageDialog");
    remap("NewInstanceGeometry", "UIGeometry/NewInstance");
    remap("NewsGeometry", "UIGeometry/News");
    remap("ModDownloadGeometry", "UIGeometry/ModDownload");
    remap("RPDownloadGeometry", "UIGeometry/RPDownload");
    remap("TPDownloadGeometry", "UIGeometry/TPDownload");
    remap("ShaderDownloadGeometry", "UIGeometry/ShaderDownload");
    remap("DataPackDownloadGeometry", "UIGeometry/DataPackDownload");

    remap("MainWindowState", "UIState/MainWindow");
    remap("ConsoleWindowState", "UIState/ConsoleWindow");

    auto remapResourcePage = [&file, &remap](const QString& name) {
        remap("UI/" + name + "_Page/ColumnSizes", "UIColumnSizes/" + name);
        if (file.value("UI/" + name + "_Page/ColumnsOverride").toBool()) {
            remap("UI/" + name + "_Page/ColumnsVisibility", "UIColumnVisibility/" + name);
        }
    };

    remapResourcePage("resource");
    remapResourcePage("mods");
    remapResourcePage("resourcepacks");
    remapResourcePage("texturepacks");
    remapResourcePage("shaderpacks");
    remapResourcePage("datapacks");
}
}  // namespace

Result<> INIFile::loadFile(const QString& fileName)
{
    QSettings settingsObj{ fileName, QSettings::Format::IniFormat };
    settingsObj.setFallbacksEnabled(false);

    if (auto status = settingsObj.status(); status != QSettings::Status::NoError) {
        if (status == QSettings::Status::AccessError) {
            return std::unexpected("Access error occurred while loading INI file from " + fileName);
        }
        if (status == QSettings::Status::FormatError) {
            return std::unexpected("Format error occurred while loading INI file from " + fileName);
        }
        return std::unexpected("Unknown error occurred while loading INI file from " + fileName);
    }
    QString configVersion = settingsObj.value("ConfigVersion").toString();
    if (configVersion.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            return std::unexpected("Failed to open INI file from " + fileName + " for reading: " + file.errorString());
        }
        QSettings::SettingsMap map;
        parseOldFileFormat(file, map);
        file.close();
        for (auto&& key : map.keys()) {
            insert(key, map.value(key));
        }
        // unquote is already called by parseOldFileFormat
        configVersion = "1.2";
    } else {
        for (auto&& key : settingsObj.allKeys()) {
            insert(key, settingsObj.value(key));
        }
    }

    if (configVersion == "1.1") {
        for (auto&& key : keys()) {
            insert(key, unquote(value(key).toString()));
        }
        configVersion = "1.2";
    }

    if (configVersion == "1.2") {
        for (auto&& key : keys()) {
            auto val = migrateQByteArrayToBase64(key, value(key));
            insert(key, val);
        }
        configVersion = "1.3";
    }

    if (configVersion == "1.3") {
        migrateUIKeys(*this);
        configVersion = "1.4";
    }

    insert("ConfigVersion", configVersion);

    return {};
}

Result<> INIFile::loadFile(const QByteArray& data)
{
    QTemporaryFile file;
    if (!file.open()) {
        return std::unexpected("Failed to open temporary file for writing: " + file.errorString());
    }
    file.write(data);
    file.flush();
    file.close();
    auto loaded = loadFile(file.fileName());
    file.remove();
    return loaded;
}

QVariant INIFile::get(const QString& key, QVariant def) const
{
    if (!this->contains(key)) {
        return def;
    }
    return this->operator[](key);
}

void INIFile::set(const QString& key, QVariant val)
{
    this->operator[](key) = std::move(val);
}
