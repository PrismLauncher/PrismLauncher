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
    : Task(parent), m_capeId(cape), m_token(token)
{
}

void CapeChange::setCape(QString& cape) {
    QNetworkRequest request(QUrl("https://api.minecraftservices.com/minecraft/profile/capes/active"));
    auto requestString = QString("{\"capeId\":\"%1\"}").arg(m_capeId);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_token).toLocal8Bit());
    QNetworkReply *rep = APPLICATION->network()->put(request, requestString.toUtf8());

    setStatus(tr("Equipping cape"));

    m_reply = shared_qobject_ptr<QNetworkReply>(rep);
    connect(rep, &QNetworkReply::uploadProgress, this, &CapeChange::setProgress);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0) // QNetworkReply::errorOccurred added in 5.15
    connect(rep, &QNetworkReply::errorOccurred, this, &CapeChange::downloadError);
#else
    connect(rep, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), this, &CapeChange::downloadError);
#endif
    connect(rep, &QNetworkReply::sslErrors, this, &CapeChange::sslErrors);
    connect(rep, &QNetworkReply::finished, this, &CapeChange::downloadFinished);
}

void CapeChange::clearCape() {
    QNetworkRequest request(QUrl("https://api.minecraftservices.com/minecraft/profile/capes/active"));
    auto requestString = QString("{\"capeId\":\"%1\"}").arg(m_capeId);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_token).toLocal8Bit());
    QNetworkReply *rep = APPLICATION->network()->deleteResource(request);

    setStatus(tr("Removing cape"));

    m_reply = shared_qobject_ptr<QNetworkReply>(rep);
    connect(rep, &QNetworkReply::uploadProgress, this, &CapeChange::setProgress);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0) // QNetworkReply::errorOccurred added in 5.15
    connect(rep, &QNetworkReply::errorOccurred, this, &CapeChange::downloadError);
#else
    connect(rep, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), this, &CapeChange::downloadError);
#endif
    connect(rep, &QNetworkReply::sslErrors, this, &CapeChange::sslErrors);
    connect(rep, &QNetworkReply::finished, this, &CapeChange::downloadFinished);
}


void CapeChange::executeTask()
{
    if(m_capeId.isEmpty()) {
        clearCape();
    }
    else {
        setCape(m_capeId);
    }
}

void CapeChange::downloadError(QNetworkReply::NetworkError error)
{
    // error happened during download.
    qCritical() << "Network error: " << error;
    emitFailed(m_reply->errorString());
}

void CapeChange::sslErrors(const QList<QSslError>& errors)
{
    int i = 1;
    for (auto error : errors) {
        qCritical() << "Cape change SSL Error #" << i << " : " << error.errorString();
        auto cert = error.certificate();
        qCritical() << "Certificate in question:\n" << cert.toText();
        i++;
    }
}

void CapeChange::downloadFinished()
{
    // if the download failed
    if (m_reply->error() != QNetworkReply::NetworkError::NoError)
    {
        emitFailed(QString("Network error: %1").arg(m_reply->errorString()));
        m_reply.reset();
        return;
    }
    emitSucceeded();
}
