// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include "ProfileUtils.h"
#include <QDebug>
#include "Json.h"
#include "minecraft/OneSixVersionFormat.h"
#include "minecraft/VersionFilterData.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>

namespace ProfileUtils {

static const int currentOrderFileVersion = 1;

bool readOverrideOrders(QString path, PatchOrder& order)
{
    QFile orderFile(path);
    if (!orderFile.exists()) {
        qWarning() << "Order file doesn't exist. Ignoring.";
        return false;
    }
    if (!orderFile.open(QFile::ReadOnly)) {
        qCritical() << "Couldn't open" << orderFile.fileName() << " for reading:" << orderFile.errorString();
        qWarning() << "Ignoring overridden order";
        return false;
    }

    // and it's valid JSON
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(orderFile.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qCritical() << "Couldn't parse" << orderFile.fileName() << ":" << error.errorString();
        qWarning() << "Ignoring overridden order";
        return false;
    }

    // and then read it and process it if all above is true.
    try {
        auto obj = Json::requireObject(doc);
        // check order file version.
        auto version = Json::requireInteger(obj.value("version"));
        if (version != currentOrderFileVersion) {
            throw JSONValidationError(QObject::tr("Invalid order file version, expected %1").arg(currentOrderFileVersion));
        }
        auto orderArray = Json::requireArray(obj.value("order"));
        for (auto item : orderArray) {
            order.append(Json::requireString(item));
        }
    } catch ([[maybe_unused]] const JSONValidationError& err) {
        qCritical() << "Couldn't parse" << orderFile.fileName() << ": bad file format";
        qWarning() << "Ignoring overridden order";
        order.clear();
        return false;
    }
    return true;
}

static VersionFilePtr createErrorVersionFile(QString fileId, QString filepath, QString error)
{
    auto outError = std::make_shared<VersionFile>();
    outError->uid = outError->name = fileId;
    // outError->filename = filepath;
    outError->addProblem(ProblemSeverity::Error, error);
    return outError;
}

static VersionFilePtr guardedParseJson(const QJsonDocument& doc, const QString& fileId, const QString& filepath, const bool& requireOrder)
{
    try {
        return OneSixVersionFormat::versionFileFromJson(doc, filepath, requireOrder);
    } catch (const Exception& e) {
        return createErrorVersionFile(fileId, filepath, e.cause());
    }
}

VersionFilePtr parseJsonFile(const QFileInfo& fileInfo, const bool requireOrder)
{
    QFile file(fileInfo.absoluteFilePath());
    if (!file.open(QFile::ReadOnly)) {
        auto errorStr = QObject::tr("Unable to open the version file %1: %2.").arg(fileInfo.fileName(), file.errorString());
        return createErrorVersionFile(fileInfo.completeBaseName(), fileInfo.absoluteFilePath(), errorStr);
    }
    QJsonParseError error;
    auto data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    file.close();
    if (error.error != QJsonParseError::NoError) {
        int line = 1;
        int column = 0;
        for (int i = 0; i < error.offset; i++) {
            if (data[i] == '\n') {
                line++;
                column = 0;
                continue;
            }
            column++;
        }
        auto errorStr = QObject::tr("Unable to process the version file %1: %2 at line %3 column %4.")
                            .arg(fileInfo.fileName(), error.errorString())
                            .arg(line)
                            .arg(column);
        return createErrorVersionFile(fileInfo.completeBaseName(), fileInfo.absoluteFilePath(), errorStr);
    }
    return guardedParseJson(doc, fileInfo.completeBaseName(), fileInfo.absoluteFilePath(), requireOrder);
}

bool saveJsonFile(const QJsonDocument& doc, const QString& filename)
{
    auto data = doc.toJson();
    QSaveFile jsonFile(filename);
    if (!jsonFile.open(QIODevice::WriteOnly)) {
        jsonFile.cancelWriting();
        qWarning() << "Couldn't open" << filename << "for writing";
        return false;
    }
    jsonFile.write(data);
    if (!jsonFile.commit()) {
        qWarning() << "Couldn't save" << filename;
        return false;
    }
    return true;
}

void removeLwjglFromPatch(VersionFilePtr patch)
{
    auto filter = [](QList<LibraryPtr>& libs) {
        QList<LibraryPtr> filteredLibs;
        for (auto lib : libs) {
            if (!g_VersionFilterData.lwjglWhitelist.contains(lib->artifactPrefix())) {
                filteredLibs.append(lib);
            }
        }
        libs = filteredLibs;
    };
    filter(patch->libraries);
}
}  // namespace ProfileUtils
