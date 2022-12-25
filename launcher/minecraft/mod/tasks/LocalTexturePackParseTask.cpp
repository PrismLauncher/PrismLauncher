// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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
 */

#include "LocalTexturePackParseTask.h"

#include "FileSystem.h"

#include <quazip/quazip.h>
#include <quazip/quazipfile.h>

#include <QCryptographicHash>

namespace TexturePackUtils {

bool process(TexturePack& pack, ProcessingLevel level)
{
    switch (pack.type()) {
        case ResourceType::FOLDER:
            return TexturePackUtils::processFolder(pack, level);
        case ResourceType::ZIPFILE:
            return TexturePackUtils::processZIP(pack, level);
        default:
            qWarning() << "Invalid type for resource pack parse task!";
            return false;
    }
}

bool processFolder(TexturePack& pack, ProcessingLevel level)
{
    Q_ASSERT(pack.type() == ResourceType::FOLDER);

    QFileInfo mcmeta_file_info(FS::PathCombine(pack.fileinfo().filePath(), "pack.txt"));
    if (mcmeta_file_info.isFile()) {
        QFile mcmeta_file(mcmeta_file_info.filePath());
        if (!mcmeta_file.open(QIODevice::ReadOnly))
            return false;

        auto data = mcmeta_file.readAll();

        bool packTXT_result = TexturePackUtils::processPackTXT(pack, std::move(data));

        mcmeta_file.close();
        if (!packTXT_result) {
            return false;
        }
    } else {
        return false;
    }

    if (level == ProcessingLevel::BasicInfoOnly)
        return true;

    QFileInfo image_file_info(FS::PathCombine(pack.fileinfo().filePath(), "pack.png"));
    if (image_file_info.isFile()) {
        QFile mcmeta_file(image_file_info.filePath());
        if (!mcmeta_file.open(QIODevice::ReadOnly))
            return false;

        auto data = mcmeta_file.readAll();

        bool packPNG_result = TexturePackUtils::processPackPNG(pack, std::move(data));

        mcmeta_file.close();
        if (!packPNG_result) {
            return false;
        }
    } else {
        return false;
    }

    return true;
}

bool processZIP(TexturePack& pack, ProcessingLevel level)
{
    Q_ASSERT(pack.type() == ResourceType::ZIPFILE);

    QuaZip zip(pack.fileinfo().filePath());
    if (!zip.open(QuaZip::mdUnzip))
        return false;

    QuaZipFile file(&zip);

    if (zip.setCurrentFile("pack.txt")) {
        if (!file.open(QIODevice::ReadOnly)) {
            qCritical() << "Failed to open file in zip.";
            zip.close();
            return false;
        }

        auto data = file.readAll();

        bool packTXT_result = TexturePackUtils::processPackTXT(pack, std::move(data));

        file.close();
        if (!packTXT_result) {
            return false;
        }
    }

    if (level == ProcessingLevel::BasicInfoOnly) {
        zip.close();
        return true;
    }

    if (zip.setCurrentFile("pack.png")) {
        if (!file.open(QIODevice::ReadOnly)) {
            qCritical() << "Failed to open file in zip.";
            zip.close();
            return false;
        }

        auto data = file.readAll();

        bool packPNG_result = TexturePackUtils::processPackPNG(pack, std::move(data));

        file.close();
        if (!packPNG_result) {
            return false;
        }
    }

    zip.close();

    return true;
}

bool processPackTXT(TexturePack& pack, QByteArray&& raw_data)
{
    pack.setDescription(QString(raw_data));
    return true;
}

bool processPackPNG(TexturePack& pack, QByteArray&& raw_data)
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

bool validate(QFileInfo file)
{
    TexturePack rp{ file };
    return TexturePackUtils::process(rp, ProcessingLevel::BasicInfoOnly) && rp.valid();
}

}  // namespace TexturePackUtils

LocalTexturePackParseTask::LocalTexturePackParseTask(int token, TexturePack& rp)
    : Task(nullptr, false), m_token(token), m_texture_pack(rp)
{}

bool LocalTexturePackParseTask::abort()
{
    m_aborted = true;
    return true;
}

void LocalTexturePackParseTask::executeTask()
{
    if (!TexturePackUtils::process(m_texture_pack))
        return;

    if (m_aborted)
        emitAborted();
    else
        emitSucceeded();
}
