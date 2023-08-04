// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
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
 */

#include "net/ApiDownload.h"
#include "ByteArraySink.h"
#include "ChecksumValidator.h"
#include "MetaCacheSink.h"
#include "net/NetAction.h"

namespace Net {

auto ApiDownload::makeCached(QUrl url, MetaEntryPtr entry, Options options) -> Download::Ptr
{
    auto dl = makeShared<ApiDownload>();
    dl->m_url = url;
    dl->setObjectName(QString("CACHE:") + url.toString());
    dl->m_options = options;
    auto md5Node = new ChecksumValidator(QCryptographicHash::Md5);
    auto cachedNode = new MetaCacheSink(entry, md5Node, options.testFlag(Option::MakeEternal));
    dl->m_sink.reset(cachedNode);
    return dl;
}

auto ApiDownload::makeByteArray(QUrl url, std::shared_ptr<QByteArray> output, Options options) -> Download::Ptr
{
    auto dl = makeShared<ApiDownload>();
    dl->m_url = url;
    dl->setObjectName(QString("BYTES:") + url.toString());
    dl->m_options = options;
    dl->m_sink.reset(new ByteArraySink(output));
    return dl;
}

auto ApiDownload::makeFile(QUrl url, QString path, Options options) -> Download::Ptr
{
    auto dl = makeShared<ApiDownload>();
    dl->m_url = url;
    dl->setObjectName(QString("FILE:") + url.toString());
    dl->m_options = options;
    dl->m_sink.reset(new FileSink(path));
    return dl;
}

void ApiDownload::init()
{
    qDebug() << "Setting up api download";
    auto api_headers = new ApiHeaderProxy();
    addHeaderProxy(api_headers);
}
}  // namespace Net
