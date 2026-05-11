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
#include <bit>

bool GZip::unzip(const QByteArray& compressedBytes, QByteArray& uncompressedBytes)
{
    if (compressedBytes.size() == 0) {
        uncompressedBytes = compressedBytes;
        return true;
    }

    auto uncompLength = static_cast<uLong>(compressedBytes.size());
    uncompressedBytes.clear();
    uncompressedBytes.resize(static_cast<qsizetype>(uncompLength));

    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    strm.next_in = std::bit_cast<Bytef*>(compressedBytes.data());
    strm.avail_in = static_cast<uLong>(compressedBytes.size());

    bool done = false;

    if (inflateInit2(&strm, (16 + MAX_WBITS)) != Z_OK) {
        return false;
    }

    int err = Z_OK;

    while (!done) {
        // If our output buffer is too small
        if (strm.total_out >= uncompLength) {
            uncompressedBytes.resize(static_cast<qsizetype>(uncompLength) * 2);
            uncompLength *= 2;
        }

        strm.next_out = std::bit_cast<Bytef*>(&uncompressedBytes[static_cast<qsizetype>(strm.total_out)]);
        strm.avail_out = uncompLength - strm.total_out;

        // Inflate another chunk.
        err = inflate(&strm, Z_SYNC_FLUSH);
        if (err == Z_STREAM_END) {
            done = true;
        } else if (err != Z_OK) {
            break;
        }
    }

    if (inflateEnd(&strm) != Z_OK || !done) {
        return false;
    }

    uncompressedBytes.resize(static_cast<qsizetype>(strm.total_out));
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

    zs.next_in = std::bit_cast<Bytef*>(uncompressedBytes.data());
    zs.avail_in = static_cast<uInt>(uncompressedBytes.size());

    int ret = 0;
    compressedBytes.resize(uncompressedBytes.size());

    unsigned offset = 0;
    unsigned temp = 0;
    while (true) {
        auto remaining = compressedBytes.size() - offset;
        if (remaining < 1) {
            compressedBytes.resize(compressedBytes.size() * 2);
        }
        zs.next_out = std::bit_cast<Bytef*>(&compressedBytes[static_cast<qsizetype>(offset)]);
        temp = zs.avail_out = static_cast<uInt>(compressedBytes.size() - offset);
        ret = deflate(&zs, Z_FINISH);
        offset += temp - zs.avail_out;
        if (ret != Z_OK) {
            break;
        }
    }

    compressedBytes.resize(offset);

    if (deflateEnd(&zs) != Z_OK) {
        return false;
    }

    if (ret != Z_STREAM_END) {
        return false;
    }
    return true;
}

namespace {
int inf(QFile* source, const std::function<bool(const QByteArray&)>& handleBlock)
{
    constexpr auto chunk = 16384;
    int ret = 0;
    unsigned have = 0;
    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    std::array<char, static_cast<std::size_t>(chunk)> in{};
    std::array<unsigned char, static_cast<std::size_t>(chunk)> out{};

    ret = inflateInit2(&strm, (16 + MAX_WBITS));
    if (ret != Z_OK) {
        return ret;
    }

    /* decompress until deflate stream ends or end of file */
    while (true) {
        strm.avail_in = static_cast<uInt>(source->read(in.data(), chunk));
        if (source->error() != 0U) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0) {
            break;
        }
        strm.next_in = std::bit_cast<Bytef*>(in.data());

        /* run inflate() on input until output buffer not full */
        while (true) {
            strm.avail_out = static_cast<uInt>(chunk);
            strm.next_out = out.data();
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR); /* state not clobbered */
            switch (ret) {
                case Z_NEED_DICT:
                    ret = Z_DATA_ERROR;
                    [[fallthrough]];
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    (void)inflateEnd(&strm);
                    return ret;
                default:
                    break;
            }
            have = static_cast<unsigned>(chunk) - strm.avail_out;
            if (!handleBlock(QByteArray(std::bit_cast<const char*>(out.data()), static_cast<qsizetype>(have)))) {
                (void)inflateEnd(&strm);
                return Z_OK;
            }

            if (strm.avail_out != 0) {
                break;
            }
        }

        /* done when inflate() says it's done */
        if (ret == Z_STREAM_END) {
            break;
        }
    }

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
        default:
            break;
    }
    return {};
}
}  // namespace

QString GZip::readGzFileByBlocks(QFile* source, const std::function<bool(const QByteArray&)>& handleBlock)
{
    auto ret = inf(source, handleBlock);
    return zerr(ret);
}
