// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2025 Trial97 <alexandru.tripon97@gmail.com>
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
#pragma once

#include <QByteArray>
#include <QFileDevice>
#include "archive/ArchiveReader.h"

struct archive;
namespace MMCZip {

class ArchiveWriter {
   public:
    ArchiveWriter(const QString& archiveName);
    virtual ~ArchiveWriter();

    bool open();
    bool close();

    bool addFile(const QString& fileName, const QString& fileDest);
    bool addFile(const QString& fileDest, const QByteArray& data);
    bool addFile(ArchiveReader::File* f);

    static std::unique_ptr<archive, void (*)(archive*)> createDiskWriter();

   private:
    struct archive* m_archive = nullptr;
    QString m_filename;
    QString m_format = "zip";
};
}  // namespace MMCZip
