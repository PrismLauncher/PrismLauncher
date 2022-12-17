// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
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

#include "CapeChange.h"

#include <QNetworkRequest>
#include <QHttpMultiPart>

#include "Application.h"

CapeChange::CapeChange(QObject *parent, QString token, QString cape)
    : Task(parent), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_capeId(cape), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_token(token)
{
}

void CapeChange::setCape(QString& cape) {
    QNetworkRequest request(QUrl("https://api.minecraftservices.com/minecraft/profile/capes/active"));
    auto requestString = QString("{\"capeId\":\"%1\"}").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_capeId);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_token).toLocal8Bit());
    QNetworkReply *rep = APPLICATION->network()->put(request, requestString.toUtf8());

    setStatus(tr("Equipping cape"));

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply = shared_qobject_ptr<QNetworkReply>(rep);
    connect(rep, &QNetworkReply::uploadProgress, this, &Task::setProgress);
    connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));
    connect(rep, SIGNAL(finished()), this, SLOT(downloadFinished()));
}

void CapeChange::clearCape() {
    QNetworkRequest request(QUrl("https://api.minecraftservices.com/minecraft/profile/capes/active"));
    auto requestString = QString("{\"capeId\":\"%1\"}").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_capeId);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_token).toLocal8Bit());
    QNetworkReply *rep = APPLICATION->network()->deleteResource(request);

    setStatus(tr("Removing cape"));

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply = shared_qobject_ptr<QNetworkReply>(rep);
    connect(rep, &QNetworkReply::uploadProgress, this, &Task::setProgress);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(rep, SIGNAL(errorOccurred(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));
#else
    connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));
#endif
    connect(rep, SIGNAL(finished()), this, SLOT(downloadFinished()));
}


void CapeChange::executeTask()
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_capeId.isEmpty()) {
        clearCape();
    }
    else {
        setCape(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_capeId);
    }
}

void CapeChange::downloadError(QNetworkReply::NetworkError error)
{
    // error happened during download.
    qCritical() << "Network error: " << error;
    emitFailed(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply->errorString());
}

void CapeChange::downloadFinished()
{
    // if the download failed
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply->error() != QNetworkReply::NetworkError::NoError)
    {
        emitFailed(QString("Network error: %1").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply->errorString()));
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply.reset();
        return;
    }
    emitSucceeded();
}
