// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

#include "SkinUpload.h"

#include <QHttpMultiPart>

#include "net/ByteArraySink.h"
#include "net/StaticHeaderProxy.h"

SkinUpload::SkinUpload(QString token, QByteArray skin, SkinUpload::Model model) : NetRequest(), m_model(model), m_skin(skin), m_token(token)
{
    logCat = taskMCServicesLogC;
};

QNetworkReply* SkinUpload::getReply(QNetworkRequest& request)
{
    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart skin;
    skin.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/png"));
    skin.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"skin.png\""));
    skin.setBody(m_skin);

    QHttpPart model;
    model.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"variant\""));

    switch (m_model) {
        default:
            qDebug() << "Unknown skin type!";
            emitFailed("Unknown skin type!");
            return nullptr;
        case SkinUpload::STEVE:
            model.setBody("CLASSIC");
            break;
        case SkinUpload::ALEX:
            model.setBody("SLIM");
            break;
    }

    multiPart->append(skin);
    multiPart->append(model);
    setStatus(tr("Uploading skin"));
    return m_network->post(request, multiPart);
}

void SkinUpload::init()
{
    addHeaderProxy(new Net::StaticHeaderProxy(QList<Net::HeaderPair>{
        { "Authorization", QString("Bearer %1").arg(m_token).toLocal8Bit() },
    }));
}

SkinUpload::Ptr SkinUpload::make(QString token, QByteArray skin, SkinUpload::Model model)
{
    auto up = makeShared<SkinUpload>(token, skin, model);
    up->m_url = QUrl("https://api.minecraftservices.com/minecraft/profile/skins");
    up->m_sink.reset(new Net::ByteArraySink(std::make_shared<QByteArray>()));
    return up;
}
