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

bool process(TexturePack& pack)
{
    switch (pack.type()) {
        case ResourceType::FOLDER:
            TexturePackUtils::processFolder(pack);
            return true;
        case ResourceType::ZIPFILE:
            TexturePackUtils::processZIP(pack);
            return true;
        default:
            qCWarning(LAUNCHER_LOG) << "Invalid type for resource pack parse task!";
            return false;
    }
}

void processFolder(TexturePack& pack)
{
    Q_ASSERT(pack.type() == ResourceType::FOLDER);

    QFileInfo mcmeta_file_info(FS::PathCombine(pack.fileinfo().filePath(), "pack.txt"));
    if (mcmeta_file_info.isFile()) {
        QFile mcmeta_file(mcmeta_file_info.filePath());
        if (!mcmeta_file.open(QIODevice::ReadOnly))
            return;

        auto data = mcmeta_file.readAll();

        TexturePackUtils::processPackTXT(pack, std::move(data));

        mcmeta_file.close();
    }

    QFileInfo image_file_info(FS::PathCombine(pack.fileinfo().filePath(), "pack.png"));
    if (image_file_info.isFile()) {
        QFile mcmeta_file(image_file_info.filePath());
        if (!mcmeta_file.open(QIODevice::ReadOnly))
            return;

        auto data = mcmeta_file.readAll();

        TexturePackUtils::processPackPNG(pack, std::move(data));

        mcmeta_file.close();
    }
}

void processZIP(TexturePack& pack)
{
    Q_ASSERT(pack.type() == ResourceType::ZIPFILE);

    QuaZip zip(pack.fileinfo().filePath());
    if (!zip.open(QuaZip::mdUnzip))
        return;

    QuaZipFile file(&zip);

    if (zip.setCurrentFile("pack.txt")) {
        if (!file.open(QIODevice::ReadOnly)) {
            qCCritical(LAUNCHER_LOG) << "Failed to open file in zip.";
            zip.close();
            return;
        }

        auto data = file.readAll();

        TexturePackUtils::processPackTXT(pack, std::move(data));

        file.close();
    }

    if (zip.setCurrentFile("pack.png")) {
        if (!file.open(QIODevice::ReadOnly)) {
            qCCritical(LAUNCHER_LOG) << "Failed to open file in zip.";
            zip.close();
            return;
        }

        auto data = file.readAll();

        TexturePackUtils::processPackPNG(pack, std::move(data));

        file.close();
    }

    zip.close();
}

void processPackTXT(TexturePack& pack, QByteArray&& raw_data)
{
    pack.setDescription(QString(raw_data));
}

void processPackPNG(TexturePack& pack, QByteArray&& raw_data)
{
    auto img = QImage::fromData(raw_data);
    if (!img.isNull()) {
        pack.setImage(img);
    } else {
        qCWarning(LAUNCHER_LOG) << "Failed to parse pack.png.";
    }
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
    Q_ASSERT(m_texture_pack.valid());

    if (!TexturePackUtils::process(m_texture_pack))
        return;

    if (m_aborted)
        emitAborted();
    else
        emitSucceeded();
}
