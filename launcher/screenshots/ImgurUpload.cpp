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

ImgurUpload::ImgurUpload(ScreenShot::Ptr shot) : NetAction(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_shot(shot)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url = BuildConfig.IMGUR_BASE_URL + "upload.json";
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state = State::Inactive;
}

void ImgurUpload::executeTask()
{
    finished = false;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state = Task::State::Running;
    QNetworkRequest request(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url);
    request.setHeader(QNetworkRequest::UserAgentHeader, APPLICATION->getUserAgentUncached().toUtf8());
    request.setRawHeader("Authorization", QString("Client-ID %1").arg(BuildConfig.IMGUR_CLIENT_ID).toStdString().c_str());
    request.setRawHeader("Accept", "application/json");

    QFile f(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_shot->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_file.absoluteFilePath());
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
    namePart.setBody(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_shot->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_file.baseName().toUtf8());
    multipart->append(namePart);

    QNetworkReply *rep = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network->post(request, multipart);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply.reset(rep);
    connect(rep, &QNetworkReply::uploadProgress, this, &ImgurUpload::downloadProgress);
    connect(rep, &QNetworkReply::finished, this, &ImgurUpload::downloadFinished);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(rep, SIGNAL(errorOccurred(QNetworkReply::NetworkError)), SLOT(downloadError(QNetworkReply::NetworkError)));
#else
    connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(downloadError(QNetworkReply::NetworkError)));
#endif
}
void ImgurUpload::downloadError(QNetworkReply::NetworkError error)
{
    qCritical() << "ImgurUpload failed with error" << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply->errorString() << "Server reply:\n" << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply->readAll();
    if(finished)
    {
        qCritical() << "Double finished ImgurUpload!";
        return;
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state = Task::State::Failed;
    finished = true;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply.reset();
    emitFailed();
}
void ImgurUpload::downloadFinished()
{
    if(finished)
    {
        qCritical() << "Double finished ImgurUpload!";
        return;
    }
    QByteArray data = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply->readAll();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply.reset();
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error != QJsonParseError::NoError)
    {
        qDebug() << "imgur server did not reply with JSON" << jsonError.errorString();
        finished = true;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply.reset();
        emitFailed();
        return;
    }
    auto object = doc.object();
    if (!object.value("success").toBool())
    {
        qDebug() << "Screenshot upload not successful:" << doc.toJson();
        finished = true;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply.reset();
        emitFailed();
        return;
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_shot->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_imgurId = object.value("data").toObject().value("id").toString();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_shot->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url = object.value("data").toObject().value("link").toString();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_shot->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_imgurDeleteHash = object.value("data").toObject().value("deletehash").toString();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state = Task::State::Succeeded;
    finished = true;
    emit succeeded();
    return;
}
void ImgurUpload::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    setProgress(bytesReceived, bytesTotal);
    emit progress(bytesReceived, bytesTotal);
}
