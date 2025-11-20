// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023-2024 Trial97 <alexandru.tripon97@gmail.com>
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
#include "Untar.h"
#include <quagzipfile.h>
#include <QByteArray>
#include <QFileInfo>
#include <QIODevice>
#include <QString>
#include "FileSystem.h"

// adaptation of the:
// - https://github.com/madler/zlib/blob/develop/contrib/untgz/untgz.c
// - https://en.wikipedia.org/wiki/Tar_(computing)
// - https://github.com/euroelessar/cutereader/blob/master/karchive/src/ktar.cpp

#define BLOCKSIZE 512
#define SHORTNAMESIZE 100

enum class TypeFlag : char {
    Regular = '0',     // regular file
    ARegular = 0,      // regular file
    Link = '1',        // link
    Symlink = '2',     // reserved
    Character = '3',   // character special
    Block = '4',       // block special
    Directory = '5',   // directory
    FIFO = '6',        // FIFO special
    Contiguous = '7',  // reserved
    // Posix stuff
    GlobalPosixHeader = 'g',
    ExtendedPosixHeader = 'x',
    // 'A'– 'Z' Vendor specific extensions(POSIX .1 - 1988)
    // GNU
    GNULongLink = 'K', /* long link name */
    GNULongName = 'L', /* long file name */
};

// struct Header {         /* byte offset */
//     char name[100];     /*   0 */
//     char mode[8];       /* 100 */
//     char uid[8];        /* 108 */
//     char gid[8];        /* 116 */
//     char size[12];      /* 124 */
//     char mtime[12];     /* 136 */
//     char chksum[8];     /* 148 */
//     TypeFlag typeflag;  /* 156 */
//     char linkname[100]; /* 157 */
//     char magic[6];      /* 257 */
//     char version[2];    /* 263 */
//     char uname[32];     /* 265 */
//     char gname[32];     /* 297 */
//     char devmajor[8];   /* 329 */
//     char devminor[8];   /* 337 */
//     char prefix[155];   /* 345 */
//                         /* 500 */
// };

bool readLonglink(QIODevice* in, qint64 size, QByteArray& longlink)
{
    qint64 n = 0;
    size--;  // ignore trailing null
    if (size < 0) {
        qCritical() << "The filename size is negative";
        return false;
    }
    longlink.resize(size + (BLOCKSIZE - size % BLOCKSIZE));  // make the size divisible by BLOCKSIZE
    for (qint64 offset = 0; offset < longlink.size(); offset += BLOCKSIZE) {
        n = in->read(longlink.data() + offset, BLOCKSIZE);
        if (n != BLOCKSIZE) {
            qCritical() << "The expected blocksize was not respected for the name";
            return false;
        }
    }
    longlink.truncate(qstrlen(longlink.constData()));
    return true;
}

int getOctal(char* buffer, int maxlenght, bool* ok)
{
    return QByteArray(buffer, qstrnlen(buffer, maxlenght)).toInt(ok, 8);
}

QString decodeName(char* name)
{
    return QFile::decodeName(QByteArray(name, qstrnlen(name, 100)));
}
bool Tar::extract(QIODevice* in, QString dst)
{
    char buffer[BLOCKSIZE];
    QString name, symlink, firstFolderName;
    bool doNotReset = false, ok;
    while (true) {
        auto n = in->read(buffer, BLOCKSIZE);
        if (n != BLOCKSIZE) {  // allways expect complete blocks
            qCritical() << "The expected blocksize was not respected";
            return false;
        }
        if (buffer[0] == 0) {  // end of archive
            return true;
        }
        int mode = getOctal(buffer + 100, 8, &ok) | QFile::ReadUser | QFile::WriteUser;  // hack to ensure write and read permisions
        if (!ok) {
            qCritical() << "The file mode can't be read";
            return false;
        }
        // there are names that are exactly 100 bytes long
        // and neither longlink nor \0 terminated (bug:101472)

        if (name.isEmpty()) {
            name = decodeName(buffer);
#ifndef Q_OS_MACOS
            // not done on macOS to ensure that the bundle is separated into its own folder, preventing accidental invalidation of the
            // code signature, namely for Adoptium Java
            if (!firstFolderName.isEmpty() && name.startsWith(firstFolderName)) {
                name = name.mid(firstFolderName.size());
            }
#endif
        }
        if (symlink.isEmpty())
            symlink = decodeName(buffer);
        qint64 size = getOctal(buffer + 124, 12, &ok);
        if (!ok) {
            qCritical() << "The file size can't be read";
            return false;
        }
        switch (TypeFlag(buffer[156])) {
            case TypeFlag::Regular:
                /* fallthrough */
            case TypeFlag::ARegular: {
                auto fileName = FS::PathCombine(dst, name);
                if (!FS::ensureFilePathExists(fileName)) {
                    qCritical() << "Can't ensure the file path to exist: " << fileName;
                    return false;
                }
                QFile out(fileName);
                if (!out.open(QFile::WriteOnly)) {
                    qCritical() << "Can't open file:" << fileName;
                    return false;
                }
                out.setPermissions(QFile::Permissions(mode));
                while (size > 0) {
                    QByteArray tmp(BLOCKSIZE, 0);
                    n = in->read(tmp.data(), BLOCKSIZE);
                    if (n != BLOCKSIZE) {
                        qCritical() << "The expected blocksize was not respected when reading file";
                        return false;
                    }
                    tmp.truncate(qMin(qint64(BLOCKSIZE), size));
                    out.write(tmp);
                    size -= BLOCKSIZE;
                }
                break;
            }
            case TypeFlag::Directory: {
                if (firstFolderName.isEmpty()) {
                    firstFolderName = name;
                    break;
                }
                auto folderPath = FS::PathCombine(dst, name);
                if (!FS::ensureFolderPathExists(folderPath)) {
                    qCritical() << "Can't ensure that folder exists: " << folderPath;
                    return false;
                }
                break;
            }
            case TypeFlag::GNULongLink: {
                doNotReset = true;
                QByteArray longlink;
                if (readLonglink(in, size, longlink)) {
                    symlink = QFile::decodeName(longlink.constData());
                } else {
                    qCritical() << "Failed to read long link";
                    return false;
                }
                break;
            }
            case TypeFlag::GNULongName: {
                doNotReset = true;
                QByteArray longlink;
                if (readLonglink(in, size, longlink)) {
                    name = QFile::decodeName(longlink.constData());
                } else {
                    qCritical() << "Failed to read long name";
                    return false;
                }
                break;
            }
            case TypeFlag::Link:
                /* fallthrough */
            case TypeFlag::Symlink: {
                auto fileName = FS::PathCombine(dst, name);
                if (!FS::create_link(FS::PathCombine(QFileInfo(fileName).path(), symlink), fileName)()) {  // do not use symlinks
                    qCritical() << "Can't create link for:" << fileName << " to:" << FS::PathCombine(QFileInfo(fileName).path(), symlink);
                    return false;
                }
                FS::ensureFilePathExists(fileName);
                QFile::setPermissions(fileName, QFile::Permissions(mode));
                break;
            }
            case TypeFlag::Character:
                /* fallthrough */
            case TypeFlag::Block:
                /* fallthrough */
            case TypeFlag::FIFO:
                /* fallthrough */
            case TypeFlag::Contiguous:
                /* fallthrough */
            case TypeFlag::GlobalPosixHeader:
                /* fallthrough */
            case TypeFlag::ExtendedPosixHeader:
                /* fallthrough */
            default:
                break;
        }
        if (!doNotReset) {
            name.truncate(0);
            symlink.truncate(0);
        }
        doNotReset = false;
    }
    return true;
}

bool GZTar::extract(QString src, QString dst)
{
    QuaGzipFile a(src);
    if (!a.open(QIODevice::ReadOnly)) {
        qCritical() << "Can't open tar file:" << src;
        return false;
    }
    return Tar::extract(&a, dst);
}