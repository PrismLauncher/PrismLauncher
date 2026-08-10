// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

#include "ImgurAPI.h"

#include "BuildConfig.h"
#include "net/RPCSink.h"
#include "net/RawHeaderProxy.h"

#include <QDebug>
#include <QFile>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QNetworkRequest>
#include <QStringList>
#include <QUrl>
#include <memory>

namespace ImgurAPI {

std::pair<Net::NetRequest::Ptr, QString*> makeUpload(ScreenShot::Ptr shot)
{
    auto* file = new QFile(shot->m_file.absoluteFilePath());
    if (!file->open(QFile::ReadOnly)) {
        qWarning() << "Could not open file" << shot->m_file.absoluteFilePath() << "for reading:" << file->errorString();
        file->deleteLater();
        return { nullptr, nullptr };
    }

    auto* multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    file->setParent(multipart);
    QHttpPart filePart;
    filePart.setBodyDevice(file);
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, "image/png");
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"image\"; filename=\"" + file->fileName() + "\"");
    multipart->append(filePart);
    QHttpPart typePart;
    typePart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"type\"");
    typePart.setBody("file");
    multipart->append(typePart);
    QHttpPart namePart;
    namePart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"title\"");
    namePart.setBody(shot->m_file.baseName().toUtf8());
    multipart->append(namePart);

    auto parseFunc = [shot](const QByteArray& response) -> Net::RpcSink<QString>::ParseResult {
        QJsonParseError jsonError;
        QJsonDocument doc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            qDebug() << "imgur server did not reply with JSON" << jsonError.errorString();
            return std::unexpected("Invalid json reply");
        }
        auto object = doc.object();
        if (!object.value("success").toBool()) {
            qDebug() << "Screenshot upload not successful:" << doc.toJson();
            return std::unexpected("Screenshot was not uploaded successfully");
        }
        shot->m_imgurId = object.value("data").toObject().value("id").toString();
        shot->m_url = object.value("data").toObject().value("link").toString();
        shot->m_imgurDeleteHash = object.value("data").toObject().value("deletehash").toString();
        return shot->m_url;
    };

    auto dl = Net::NetRequest::makeCustomRequest(QUrl(BuildConfig.IMGUR_BASE_URL + "image"), multipart, Net::NetRequest::HttpMethod::Post);
    multipart->setParent(dl.get());
    auto sink = std::make_unique<Net::RpcSink<QString>>(std::move(parseFunc));
    auto* result = sink->result();
    dl->setSink(std::move(sink));
    dl->addHeaderProxy(std::make_unique<Net::RawHeaderProxy>(QList<Net::HeaderPair>{
        { .headerName = "Authorization", .headerValue = QString("Client-ID %1").arg(BuildConfig.IMGUR_CLIENT_ID).toUtf8() },
        { .headerName = "Accept", .headerValue = "application/json" } }));
    return { dl, result };
}

std::pair<Net::NetRequest::Ptr, AlbumResult*> makeAlbum(const QList<ScreenShot::Ptr>& screenshots)
{
    QStringList hashes;
    for (const auto& shot : screenshots) {
        hashes.append(shot->m_imgurDeleteHash);
    }
    const QByteArray data = "deletehashes=" + hashes.join(',').toUtf8() + "&title=Minecraft%20Screenshots&privacy=hidden";

    auto parseFunc = [](const QByteArray& response) -> Net::RpcSink<AlbumResult>::ParseResult {
        QJsonParseError jsonError;
        QJsonDocument doc = QJsonDocument::fromJson(response, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            qDebug() << jsonError.errorString();
            return std::unexpected("Invalid json reply");
        }
        auto object = doc.object();
        if (!object.value("success").toBool()) {
            qDebug() << doc.toJson();
            return std::unexpected("Failed to create album");
        }
        return AlbumResult{ .deleteHash = object.value("data").toObject().value("deletehash").toString(),
                            .id = object.value("data").toObject().value("id").toString() };
    };

    auto dl = Net::NetRequest::makeCustomRequest(QUrl(BuildConfig.IMGUR_BASE_URL + "album"), data, Net::NetRequest::HttpMethod::Post);
    auto sink = std::make_unique<Net::RpcSink<AlbumResult>>(std::move(parseFunc));
    auto* result = sink->result();
    dl->setSink(std::move(sink));
    dl->addHeaderProxy(std::make_unique<Net::RawHeaderProxy>(QList<Net::HeaderPair>{
        { .headerName = "Content-Type", .headerValue = "application/x-www-form-urlencoded" },
        { .headerName = "Authorization", .headerValue = QString("Client-ID %1").arg(BuildConfig.IMGUR_CLIENT_ID).toUtf8() },
        { .headerName = "Accept", .headerValue = "application/json" } }));
    return { dl, result };
}

}  // namespace ImgurAPI
