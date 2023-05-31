// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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

#include "ImgurUpload.h"
#include "BuildConfig.h"
#include "Application.h"

#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHttpPart>
#include <QFile>
#include <QUrl>
#include <QDebug>

ImgurUpload::ImgurUpload(ScreenShot::Ptr shot) : NetAction(), m_shot(shot)
{
    m_url = BuildConfig.IMGUR_BASE_URL + "upload.json";
    m_state = State::Inactive;
}

void ImgurUpload::executeTask()
{
    finished = false;
    m_state = Task::State::Running;
    QNetworkRequest request(m_url);
    request.setHeader(QNetworkRequest::UserAgentHeader, APPLICATION->getUserAgentUncached().toUtf8());
    request.setRawHeader("Authorization", QString("Client-ID %1").arg(BuildConfig.IMGUR_CLIENT_ID).toStdString().c_str());
    request.setRawHeader("Accept", "application/json");

    QFile f(m_shot->m_file.absoluteFilePath());
    if (!f.open(QFile::ReadOnly))
    {
        emitFailed();
        return;
    }

    QHttpMultiPart *multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart filePart;
    filePart.setBody(f.readAll().toBase64());
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, "image/png");
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"image\"");
    multipart->append(filePart);
    QHttpPart typePart;
    typePart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"type\"");
    typePart.setBody("base64");
    multipart->append(typePart);
    QHttpPart namePart;
    namePart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"name\"");
    namePart.setBody(m_shot->m_file.baseName().toUtf8());
    multipart->append(namePart);

    QNetworkReply *rep = m_network->post(request, multipart);

    m_reply.reset(rep);
    connect(rep, &QNetworkReply::uploadProgress, this, &ImgurUpload::downloadProgress);
    connect(rep, &QNetworkReply::finished, this, &ImgurUpload::downloadFinished);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0) // QNetworkReply::errorOccurred added in 5.15
    connect(rep, &QNetworkReply::errorOccurred, this, &ImgurUpload::downloadError);
#else
    connect(rep, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), this, &ImgurUpload::downloadError);
#endif
    connect(rep, &QNetworkReply::sslErrors, this, &ImgurUpload::sslErrors);
}

void ImgurUpload::downloadError(QNetworkReply::NetworkError error)
{
    qCritical() << "ImgurUpload failed with error" << m_reply->errorString() << "Server reply:\n" << m_reply->readAll();
    if(finished)
    {
        qCritical() << "Double finished ImgurUpload!";
        return;
    }
    m_state = Task::State::Failed;
    finished = true;
    m_reply.reset();
    emitFailed();
}

void ImgurUpload::downloadFinished()
{
    if(finished)
    {
        qCritical() << "Double finished ImgurUpload!";
        return;
    }
    QByteArray data = m_reply->readAll();
    m_reply.reset();
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error != QJsonParseError::NoError)
    {
        qDebug() << "imgur server did not reply with JSON" << jsonError.errorString();
        finished = true;
        m_reply.reset();
        emitFailed();
        return;
    }
    auto object = doc.object();
    if (!object.value("success").toBool())
    {
        qDebug() << "Screenshot upload not successful:" << doc.toJson();
        finished = true;
        m_reply.reset();
        emitFailed();
        return;
    }
    m_shot->m_imgurId = object.value("data").toObject().value("id").toString();
    m_shot->m_url = object.value("data").toObject().value("link").toString();
    m_shot->m_imgurDeleteHash = object.value("data").toObject().value("deletehash").toString();
    m_state = Task::State::Succeeded;
    finished = true;
    emit succeeded();
    return;
}

void ImgurUpload::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    setProgress(bytesReceived, bytesTotal);
    emit progress(bytesReceived, bytesTotal);
}
