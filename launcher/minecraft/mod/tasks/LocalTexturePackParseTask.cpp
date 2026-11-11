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
#include "archive/ArchiveReader.h"

#include <QCryptographicHash>

namespace TexturePackUtils {

bool process(TexturePack& pack)
{
    switch (pack.type()) {
        case ResourceType::FOLDER:
            return TexturePackUtils::processFolder(pack);
        case ResourceType::ZIPFILE:
            return TexturePackUtils::processZIP(pack);
        default:
            qWarning() << "Invalid type for resource pack parse task!";
            return false;
    }
}

bool processFolder(TexturePack& pack)
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

    return true;
}

bool processZIP(TexturePack& pack)
{
    Q_ASSERT(pack.type() == ResourceType::ZIPFILE);

    MMCZip::ArchiveReader zip(pack.fileinfo().filePath());
    const auto data = zip.readFile("pack.txt");
    if (!data.has_value()) {
        qWarning() << "Could not read pack.txt from texture pack at" << pack.fileinfo().filePath() << "-" << data.error();
        return false;
    }

    pack.setDescription(QString::fromUtf8(data.value()));
    return true;
}

bool processPackTXT(TexturePack& pack, QByteArray&& raw_data)
{
    pack.setDescription(QString(raw_data));
    return true;
}

QPixmap processPackPNG(ResourceType type, const QString& path)
{
    auto png_invalid = [&path]() {
        qWarning() << "Texture pack at" << path << "does not have a valid pack.png";
        return QPixmap();
    };

    switch (type) {
        case ResourceType::FOLDER: {
            QFileInfo image_file_info(FS::PathCombine(path, "pack.png"));
            if (image_file_info.exists() && image_file_info.isFile()) {
                QFile pack_png_file(image_file_info.filePath());
                if (!pack_png_file.open(QIODevice::ReadOnly))
                    return png_invalid();  // can't open pack.png file

                auto data = pack_png_file.readAll();

                QPixmap result;
                if (!result.loadFromData(data)) {
                    return png_invalid();
                }

                return result;
            }

            return png_invalid();  // pack.png does not exists or is not a valid file.
        }
        case ResourceType::ZIPFILE: {
            MMCZip::ArchiveReader zip(path);

            if (auto dataRes = zip.readFile("pack.png"); dataRes) {
                QPixmap result;
                if (!result.loadFromData(dataRes.value())) {
                    return png_invalid();
                }
                return result;
            }
            return png_invalid();  // could not set pack.mcmeta as current file.
        }
        default:
            qWarning() << "Invalid type for resource pack parse task!";
            return {};
    }
}

bool validate(QFileInfo file)
{
    TexturePack rp{ file };
    return TexturePackUtils::process(rp) && rp.valid();
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
