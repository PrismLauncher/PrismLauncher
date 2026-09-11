// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

#include "FileSink.h"

#include "FileSystem.h"

#include "net/Logging.h"

namespace Net {

Task::State FileSink::init(QNetworkRequest& request)
{
    auto result = initCache(request);
    if (result != Task::State::Running) {
        return result;
    }

    // create a new save file and open it for writing
    if (!FS::ensureFilePathExists(m_filename)) {
        qCCritical(taskNetLogC) << "Could not create folder for " + m_filename;
        m_failReason = "Could not create folder";
        return Task::State::Failed;
    }

    m_wroteAnyData = false;
    m_outputFile.reset(new PSaveFile(m_filename));
    if (!m_outputFile->open(QIODevice::WriteOnly)) {
        const auto error = QString("Could not open %1 for writing: %2").arg(m_filename).arg(m_outputFile->errorString());
        qCCritical(taskNetLogC) << error;
        m_failReason = error;
        return Task::State::Failed;
    }

    initAllValidators();
    return Task::State::Running;
}

Task::State FileSink::write(const QByteArray& data)
{
    writeAllValidators(data);
    if (m_outputFile->write(data) != data.size()) {
        QString error = QString("Failed writing into %1: %2").arg(m_filename);
        if (m_outputFile->error() == QFileDevice::NoError) {
            error = error.arg("Validators failed");
        } else {
            error = error.arg(m_outputFile->errorString());
        }
        qCCritical(taskNetLogC) << error;
        m_failReason = error;
        m_outputFile->cancelWriting();
        m_outputFile.reset();
        m_wroteAnyData = false;
        return Task::State::Failed;
    }

    m_wroteAnyData = true;
    return Task::State::Running;
}

Task::State FileSink::abort()
{
    if (m_outputFile) {
        m_outputFile->cancelWriting();
    }
    failAllValidators();
    return Task::State::Failed;
}

Task::State FileSink::finalize(QNetworkReply& reply)
{
    bool gotFile = false;
    QVariant statusCodeV = reply.attribute(QNetworkRequest::HttpStatusCodeAttribute);
    bool validStatus = false;
    int statusCode = statusCodeV.toInt(&validStatus);
    if (validStatus) {
        // this leaves out 304 Not Modified
        gotFile = statusCode == 200 || statusCode == 203;
    }

    // if we wrote any data to the save file, we try to commit the data to the real file.
    // if it actually got a proper file, we write it even if it was empty
    if (gotFile || m_wroteAnyData) {
        // ask validators for data consistency
        // we only do this for actual downloads, not 'your data is still the same' cache hits
        auto result = finalizeAllValidators();
        if (!result) {
            m_failReason = result.error();
            return Task::State::Failed;
        }

        // nothing went wrong...
        if (!m_outputFile->commit()) {
            const auto error = QString("Failed to commit changes to %1: %2").arg(m_filename).arg(m_outputFile->errorString());
            qCCritical(taskNetLogC) << error;
            m_failReason = error;
            m_outputFile->cancelWriting();
            return Task::State::Failed;
        }
    }

    // then get rid of the save file
    m_outputFile.reset();

    return finalizeCache(reply);
}

Task::State FileSink::initCache(QNetworkRequest& /*unused*/)
{
    return Task::State::Running;
}

Task::State FileSink::finalizeCache(QNetworkReply& /*unused*/)
{
    return Task::State::Succeeded;
}

bool FileSink::hasLocalData()
{
    QFileInfo info(m_filename);
    return info.exists() && info.size() != 0;
}
}  // namespace Net
