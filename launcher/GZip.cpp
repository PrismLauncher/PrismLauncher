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

#include "GZip.h"
#include <zlib.h>
#include <QByteArray>
#include <QDebug>
#include <QFile>

bool GZip::unzip(const QByteArray& compressedBytes, QByteArray& uncompressedBytes)
{
    if (compressedBytes.size() == 0) {
        uncompressedBytes = compressedBytes;
        return true;
    }

    unsigned uncompLength = compressedBytes.size();
    uncompressedBytes.clear();
    uncompressedBytes.resize(uncompLength);

    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    strm.next_in = (Bytef*)compressedBytes.data();
    strm.avail_in = compressedBytes.size();

    bool done = false;

    if (inflateInit2(&strm, (16 + MAX_WBITS)) != Z_OK) {
        return false;
    }

    int err = Z_OK;

    while (!done) {
        // If our output buffer is too small
        if (strm.total_out >= uncompLength) {
            uncompressedBytes.resize(uncompLength * 2);
            uncompLength *= 2;
        }

        strm.next_out = reinterpret_cast<Bytef*>((uncompressedBytes.data() + strm.total_out));
        strm.avail_out = uncompLength - strm.total_out;

        // Inflate another chunk.
        err = inflate(&strm, Z_SYNC_FLUSH);
        if (err == Z_STREAM_END)
            done = true;
        else if (err != Z_OK) {
            break;
        }
    }

    if (inflateEnd(&strm) != Z_OK || !done) {
        return false;
    }

    uncompressedBytes.resize(strm.total_out);
    return true;
}

bool GZip::zip(const QByteArray& uncompressedBytes, QByteArray& compressedBytes)
{
    if (uncompressedBytes.size() == 0) {
        compressedBytes = uncompressedBytes;
        return true;
    }

    unsigned compLength = qMin(uncompressedBytes.size(), 16);
    compressedBytes.clear();
    compressedBytes.resize(compLength);

    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, (16 + MAX_WBITS), 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return false;
    }

    zs.next_in = (Bytef*)uncompressedBytes.data();
    zs.avail_in = uncompressedBytes.size();

    int ret;
    compressedBytes.resize(uncompressedBytes.size());

    unsigned offset = 0;
    unsigned temp = 0;
    do {
        auto remaining = compressedBytes.size() - offset;
        if (remaining < 1) {
            compressedBytes.resize(compressedBytes.size() * 2);
        }
        zs.next_out = reinterpret_cast<Bytef*>((compressedBytes.data() + offset));
        temp = zs.avail_out = compressedBytes.size() - offset;
        ret = deflate(&zs, Z_FINISH);
        offset += temp - zs.avail_out;
    } while (ret == Z_OK);

    compressedBytes.resize(offset);

    if (deflateEnd(&zs) != Z_OK) {
        return false;
    }

    if (ret != Z_STREAM_END) {
        return false;
    }
    return true;
}

int inf(QFile* source, std::function<bool(const QByteArray&)> handleBlock)
{
    constexpr auto CHUNK = 16384;
    int ret;
    unsigned have;
    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    char in[CHUNK];
    unsigned char out[CHUNK];

    ret = inflateInit2(&strm, (16 + MAX_WBITS));
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = source->read(in, CHUNK);
        if (source->error()) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = reinterpret_cast<Bytef*>(in);

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR); /* state not clobbered */
            switch (ret) {
                case Z_NEED_DICT:
                    ret = Z_DATA_ERROR; /* and fall through */
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    (void)inflateEnd(&strm);
                    return ret;
            }
            have = CHUNK - strm.avail_out;
            if (!handleBlock(QByteArray(reinterpret_cast<const char*>(out), have))) {
                (void)inflateEnd(&strm);
                return Z_OK;
            }

        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

QString zerr(int ret)
{
    switch (ret) {
        case Z_ERRNO:
            return QObject::tr("error handling file");
        case Z_STREAM_ERROR:
            return QObject::tr("invalid compression level");
        case Z_DATA_ERROR:
            return QObject::tr("invalid or incomplete deflate data");
        case Z_MEM_ERROR:
            return QObject::tr("out of memory");
        case Z_VERSION_ERROR:
            return QObject::tr("zlib version mismatch!");
    }
    return {};
}

QString GZip::readGzFileByBlocks(QFile* source, std::function<bool(const QByteArray&)> handleBlock)
{
    auto ret = inf(source, handleBlock);
    return zerr(ret);
}
