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

#include <quazip/quazip.h>
#include <quazip/quazipdir.h>
#include <quazip/quazipfile.h>

#include <QCryptographicHash>

namespace DataPackUtils {

bool process(DataPack& pack, ProcessingLevel level)
{
    switch (pack.type()) {
        case ResourceType::FOLDER:
            return DataPackUtils::processFolder(pack, level);
        case ResourceType::ZIPFILE:
            return DataPackUtils::processZIP(pack, level);
        default:
            qWarning() << "Invalid type for data pack parse task!";
            return false;
    }
}

bool processFolder(DataPack& pack, ProcessingLevel level)
{
    Q_ASSERT(pack.type() == ResourceType::FOLDER);

    auto mcmeta_invalid = [&pack]() {
        qWarning() << "Data pack at" << pack.fileinfo().filePath() << "does not have a valid pack.mcmeta";
        return false;  // the mcmeta is not optional
    };

    QFileInfo mcmeta_file_info(FS::PathCombine(pack.fileinfo().filePath(), "pack.mcmeta"));
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

    QFileInfo data_dir_info(FS::PathCombine(pack.fileinfo().filePath(), "data"));
    if (!data_dir_info.exists() || !data_dir_info.isDir()) {
        return false;  // data dir does not exists or isn't valid
    }

    if (level == ProcessingLevel::BasicInfoOnly) {
        return true;  // only need basic info already checked
    }

    return true;  // all tests passed
}

bool processZIP(DataPack& pack, ProcessingLevel level)
{
    Q_ASSERT(pack.type() == ResourceType::ZIPFILE);

    QuaZip zip(pack.fileinfo().filePath());
    if (!zip.open(QuaZip::mdUnzip))
        return false;  // can't open zip file

    QuaZipFile file(&zip);

    auto mcmeta_invalid = [&pack]() {
        qWarning() << "Data pack at" << pack.fileinfo().filePath() << "does not have a valid pack.mcmeta";
        return false;  // the mcmeta is not optional
    };

    if (zip.setCurrentFile("pack.mcmeta")) {
        if (!file.open(QIODevice::ReadOnly)) {
            qCritical() << "Failed to open file in zip.";
            zip.close();
            return mcmeta_invalid();
        }

        auto data = file.readAll();

        bool mcmeta_result = DataPackUtils::processMCMeta(pack, std::move(data));

        file.close();
        if (!mcmeta_result) {
            return mcmeta_invalid();  // mcmeta invalid
        }
    } else {
        return mcmeta_invalid();  // could not set pack.mcmeta as current file.
    }

    QuaZipDir zipDir(&zip);
    if (!zipDir.exists("/data")) {
        return false;  // data dir does not exists at zip root
    }

    if (level == ProcessingLevel::BasicInfoOnly) {
        zip.close();
        return true;  // only need basic info already checked
    }

    zip.close();

    return true;
}

// https://minecraft.wiki/w/Data_pack#pack.mcmeta
bool processMCMeta(DataPack& pack, QByteArray&& raw_data)
{
    try {
        auto json_doc = QJsonDocument::fromJson(raw_data);
        auto pack_obj = Json::requireObject(json_doc.object(), "pack", {});

        pack.setPackFormat(Json::ensureInteger(pack_obj, "pack_format", 0));
        pack.setDescription(Json::ensureString(pack_obj, "description", ""));
    } catch (Json::JsonException& e) {
        qWarning() << "JsonException: " << e.what() << e.cause();
        return false;
    }
    return true;
}

bool validate(QFileInfo file)
{
    DataPack dp{ file };
    return DataPackUtils::process(dp, ProcessingLevel::BasicInfoOnly) && dp.valid();
}

}  // namespace DataPackUtils

LocalDataPackParseTask::LocalDataPackParseTask(int token, DataPack& dp) : Task(nullptr, false), m_token(token), m_data_pack(dp) {}

bool LocalDataPackParseTask::abort()
{
    m_aborted = true;
    return true;
}

void LocalDataPackParseTask::executeTask()
{
    if (!DataPackUtils::process(m_data_pack))
        return;

    if (m_aborted)
        emitAborted();
    else
        emitSucceeded();
}
