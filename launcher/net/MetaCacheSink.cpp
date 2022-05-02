// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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

#include "MetaCacheSink.h"
#include <QFile>
#include <QFileInfo>
#include "FileSystem.h"
#include "Application.h"

namespace Net {

MetaCacheSink::MetaCacheSink(MetaEntryPtr entry, ChecksumValidator * md5sum)
    :Net::FileSink(entry->getFullPath()), m_entry(entry), m_md5Node(md5sum)
{
    addValidator(md5sum);
}

Task::State MetaCacheSink::initCache(QNetworkRequest& request)
{
    if (!m_entry->isStale())
    {
        return Task::State::Succeeded;
    }

    // check if file exists, if it does, use its information for the request
    QFile current(m_filename);
    if(current.exists() && current.size() != 0)
    {
        if (m_entry->getRemoteChangedTimestamp().size())
        {
            request.setRawHeader(QString("If-Modified-Since").toLatin1(), m_entry->getRemoteChangedTimestamp().toLatin1());
        }
        if (m_entry->getETag().size())
        {
            request.setRawHeader(QString("If-None-Match").toLatin1(), m_entry->getETag().toLatin1());
        }
    }

    return Task::State::Running;
}

Task::State MetaCacheSink::finalizeCache(QNetworkReply & reply)
{
    QFileInfo output_file_info(m_filename);

    if(wroteAnyData)
    {
        m_entry->setMD5Sum(m_md5Node->hash().toHex().constData());
    }

    m_entry->setETag(reply.rawHeader("ETag").constData());

    if (reply.hasRawHeader("Last-Modified"))
    {
        m_entry->setRemoteChangedTimestamp(reply.rawHeader("Last-Modified").constData());
    }

    m_entry->setLocalChangedTimestamp(output_file_info.lastModified().toUTC().toMSecsSinceEpoch());
    m_entry->setStale(false);
    APPLICATION->metacache()->updateEntry(m_entry);

    return Task::State::Succeeded;
}

bool MetaCacheSink::hasLocalData()
{
    QFileInfo info(m_filename);
    return info.exists() && info.size() != 0;
}
}
