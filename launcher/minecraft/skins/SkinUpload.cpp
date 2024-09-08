// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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

#include "FileSystem.h"
#include "net/ByteArraySink.h"
#include "net/RawHeaderProxy.h"

SkinUpload::SkinUpload(QString path, QString variant) : NetRequest(), m_path(path), m_variant(variant)
{
    logCat = taskMCSkinsLogC;
}

QNetworkReply* SkinUpload::getReply(QNetworkRequest& request)
{
    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType, this);

    QHttpPart skin;
    skin.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/png"));
    skin.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"skin.png\""));

    skin.setBody(FS::read(m_path));

    QHttpPart model;
    model.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"variant\""));
    model.setBody(m_variant.toUtf8());

    multiPart->append(skin);
    multiPart->append(model);
    setStatus(tr("Uploading skin"));
    return m_network->post(request, multiPart);
}

SkinUpload::Ptr SkinUpload::make(QString token, QString path, QString variant)
{
    auto up = makeShared<SkinUpload>(path, variant);
    up->m_url = QUrl("https://api.minecraftservices.com/minecraft/profile/skins");
    up->setObjectName(QString("BYTES:") + up->m_url.toString());
    up->m_sink.reset(new Net::ByteArraySink(std::make_shared<QByteArray>()));
    up->addHeaderProxy(new Net::RawHeaderProxy(QList<Net::HeaderPair>{
        { "Authorization", QString("Bearer %1").arg(token).toLocal8Bit() },
    }));
    return up;
}
