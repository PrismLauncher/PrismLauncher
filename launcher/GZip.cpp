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
