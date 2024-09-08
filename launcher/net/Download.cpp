// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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

#include "Download.h"
#include <QUrl>

#include <QDateTime>
#include <QFileInfo>
#include <memory>

#include "ByteArraySink.h"
#include "ChecksumValidator.h"
#include "MetaCacheSink.h"

namespace Net {

#if defined(LAUNCHER_APPLICATION)
auto Download::makeCached(QUrl url, MetaEntryPtr entry, Options options) -> Download::Ptr
{
    auto dl = makeShared<Download>();
    dl->m_url = url;
    dl->setObjectName(QString("CACHE:") + url.toString());
    dl->m_options = options;
    auto md5Node = new ChecksumValidator(QCryptographicHash::Md5);
    auto cachedNode = new MetaCacheSink(entry, md5Node, options.testFlag(Option::MakeEternal));
    dl->m_sink.reset(cachedNode);
    return dl;
}
#endif

auto Download::makeByteArray(QUrl url, std::shared_ptr<QByteArray> output, Options options) -> Download::Ptr
{
    auto dl = makeShared<Download>();
    dl->m_url = url;
    dl->setObjectName(QString("BYTES:") + url.toString());
    dl->m_options = options;
    dl->m_sink.reset(new ByteArraySink(output));
    return dl;
}

auto Download::makeFile(QUrl url, QString path, Options options) -> Download::Ptr
{
    auto dl = makeShared<Download>();
    dl->m_url = url;
    dl->setObjectName(QString("FILE:") + url.toString());
    dl->m_options = options;
    dl->m_sink.reset(new FileSink(path));
    return dl;
}

QNetworkReply* Download::getReply(QNetworkRequest& request)
{
    return m_network->get(request);
}
}  // namespace Net
