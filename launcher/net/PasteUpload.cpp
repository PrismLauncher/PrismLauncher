// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
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

#include "PasteUpload.h"
#include "BuildConfig.h"
#include "Application.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>

PasteUpload::PasteUpload(QWidget *window, QString text, QString url) : m_window(window), m_uploadUrl(url), m_text(text.toUtf8())
{
}

PasteUpload::~PasteUpload()
{
}

void PasteUpload::executeTask()
{
    QNetworkRequest request{QUrl(m_uploadUrl)};
    request.setHeader(QNetworkRequest::UserAgentHeader, BuildConfig.USER_AGENT_UNCACHED);

    QHttpMultiPart *multiPart = new QHttpMultiPart{QHttpMultiPart::FormDataType};

    QHttpPart filePart;
    filePart.setBody(m_text);
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"file\"; filename=\"log.txt\"");

    multiPart->append(filePart);

    QNetworkReply *rep = APPLICATION->network()->post(request, multiPart);
    multiPart->setParent(rep);

    m_reply = std::shared_ptr<QNetworkReply>(rep);
    setStatus(tr("Uploading to %1").arg(m_uploadUrl));

    connect(rep, &QNetworkReply::uploadProgress, this, &Task::setProgress);
    connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));
    connect(rep, SIGNAL(finished()), this, SLOT(downloadFinished()));
}

void PasteUpload::downloadError(QNetworkReply::NetworkError error)
{
    // error happened during download.
    qCritical() << "Network error: " << error;
    emitFailed(m_reply->errorString());
}

void PasteUpload::downloadFinished()
{
    QByteArray data = m_reply->readAll();
    int statusCode = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (m_reply->error() != QNetworkReply::NetworkError::NoError)
    {
        emitFailed(tr("Network error: %1").arg(m_reply->errorString()));
        m_reply.reset();
        return;
    }
    else if (statusCode != 200 && statusCode != 201)
    {
        QString reasonPhrase = m_reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
        emitFailed(tr("Error: %1 returned unexpected status code %2 %3").arg(m_uploadUrl).arg(statusCode).arg(reasonPhrase));
        qCritical() << m_uploadUrl << " returned unexpected status code " << statusCode << " with body: " << data;
        m_reply.reset();
        return;
    }

    m_pasteLink = QString::fromUtf8(data).trimmed();
    emitSucceeded();
}
