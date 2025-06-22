// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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
        zip.close();
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

bool processPackPNG(const TexturePack& pack, QByteArray&& raw_data)
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

bool processPackPNG(const TexturePack& pack)
{
    auto png_invalid = [&pack]() {
        qWarning() << "Texture pack at" << pack.fileinfo().filePath() << "does not have a valid pack.png";
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

                bool pack_png_result = TexturePackUtils::processPackPNG(pack, std::move(data));

                pack_png_file.close();
                if (!pack_png_result) {
                    return png_invalid();  // pack.png invalid
                }
            } else {
                return png_invalid();  // pack.png does not exists or is not a valid file.
            }
            return false;
        }
        case ResourceType::ZIPFILE: {
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

                bool pack_png_result = TexturePackUtils::processPackPNG(pack, std::move(data));

                file.close();
                if (!pack_png_result) {
                    zip.close();
                    return png_invalid();  // pack.png invalid
                }
            } else {
                zip.close();
                return png_invalid();  // could not set pack.mcmeta as current file.
            }
            return false;
        }
        default:
            qWarning() << "Invalid type for resource pack parse task!";
            return false;
    }
}

bool validate(QFileInfo file)
{
    TexturePack rp{ file };
    return TexturePackUtils::process(rp, ProcessingLevel::BasicInfoOnly) && rp.valid();
}

}  // namespace TexturePackUtils

LocalTexturePackParseTask::LocalTexturePackParseTask(int token, TexturePack& rp) : Task(false), m_token(token), m_texture_pack(rp) {}

bool LocalTexturePackParseTask::abort()
{
    m_aborted = true;
    return true;
}

void LocalTexturePackParseTask::executeTask()
{
    if (!TexturePackUtils::process(m_texture_pack)) {
        emitFailed("this is not a texture pack");
        return;
    }

    if (m_aborted)
        emitAborted();
    else
        emitSucceeded();
}
