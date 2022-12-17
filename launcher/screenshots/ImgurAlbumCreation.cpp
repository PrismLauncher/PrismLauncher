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

#include "ImgurAlbumCreation.h"

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QStringList>
#include <QDebug>

#include "BuildConfig.h"
#include "Application.h"

ImgurAlbumCreation::ImgurAlbumCreation(QList<ScreenShot::Ptr> screenshots) : NetAction(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_screenshots(screenshots)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url = BuildConfig.IMGUR_BASE_URL + "album.json";
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state = State::Inactive;
}

void ImgurAlbumCreation::executeTask()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state = State::Running;
    QNetworkRequest request(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url);
    request.setHeader(QNetworkRequest::UserAgentHeader, APPLICATION->getUserAgentUncached().toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Authorization", QString("Client-ID %1").arg(BuildConfig.IMGUR_CLIENT_ID).toStdString().c_str());
    request.setRawHeader("Accept", "application/json");

    QStringList hashes;
    for (auto shot : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_screenshots)
    {
        hashes.append(shot->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_imgurDeleteHash);
    }

    const QByteArray data = "deletehashes=" + hashes.join(',').toUtf8() + "&title=Minecraft%20Screenshots&privacy=hidden";

    QNetworkReply *rep = APPLICATION->network()->post(request, data);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply.reset(rep);
    connect(rep, &QNetworkReply::uploadProgress, this, &ImgurAlbumCreation::downloadProgress);
    connect(rep, &QNetworkReply::finished, this, &ImgurAlbumCreation::downloadFinished);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(rep, SIGNAL(errorOccurred(QNetworkReply::NetworkError)), SLOT(downloadError(QNetworkReply::NetworkError)));
#else
    connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(downloadError(QNetworkReply::NetworkError)));
#endif
}
void ImgurAlbumCreation::downloadError(QNetworkReply::NetworkError error)
{
    qDebug() << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply->errorString();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state = State::Failed;
}
void ImgurAlbumCreation::downloadFinished()
{
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state != State::Failed)
    {
        QByteArray data = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply->readAll();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply.reset();
        QJsonParseError jsonError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
        if (jsonError.error != QJsonParseError::NoError)
        {
            qDebug() << jsonError.errorString();
            emitFailed();
            return;
        }
        auto object = doc.object();
        if (!object.value("success").toBool())
        {
            qDebug() << doc.toJson();
            emitFailed();
            return;
        }
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_deleteHash = object.value("data").toObject().value("deletehash").toString();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_id = object.value("data").toObject().value("id").toString();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state = State::Succeeded;
        emit succeeded();
        return;
    }
    else
    {
        qDebug() << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply->readAll();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply.reset();
        emitFailed();
        return;
    }
}
void ImgurAlbumCreation::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    setProgress(bytesReceived, bytesTotal);
    emit progress(bytesReceived, bytesTotal);
}
