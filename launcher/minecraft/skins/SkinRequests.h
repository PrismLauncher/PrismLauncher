// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2026 Trial97 <alexandru.tripon97@gmail.com>
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
 */

#pragma once

#include <QHttpMultiPart>
#include <QHttpPart>
#include <QNetworkRequest>
#include <QUrl>
#include <memory>

#include "FileSystem.h"
#include "net/NetRequest.h"
#include "net/DummySink.h"
#include "net/RawHeaderProxy.h"

inline Net::NetRequest::Ptr makeSkinDeleteRequest(QString token)
{
    auto req = Net::NetRequest::makeCustomRequest(QUrl("https://api.minecraftservices.com/minecraft/profile/skins/active"), nullptr,
                                                  Net::NetRequest::HttpMethod::Delete, Net::NetRequest::Option::NoOptions, QString(),
                                                  taskMCSkinsLogC);
    req->setSink(std::make_unique<Net::DummySink>());
    req->addHeaderProxy(std::make_unique<Net::RawHeaderProxy>(QList<Net::HeaderPair>{
        { .headerName = "Authorization", .headerValue = QString("Bearer %1").arg(token).toLocal8Bit() },
    }));
    return req;
}

inline Net::NetRequest::Ptr makeSkinUploadRequest(QString token, QString path, QString variant)
{
    auto multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart skin;
    skin.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/png"));
    skin.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(R"(form-data; name="file"; filename="skin.png")"));
    skin.setBody(FS::read(path));

    QHttpPart model;
    model.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"variant\""));
    model.setBody(variant.toUtf8());

    multiPart->append(skin);
    multiPart->append(model);

    auto req = Net::NetRequest::makeCustomRequest(QUrl("https://api.minecraftservices.com/minecraft/profile/skins"), multiPart,
                                                  Net::NetRequest::HttpMethod::Post, Net::NetRequest::Option::NoOptions, QString(),
                                                  taskMCSkinsLogC);
    req->setSink(std::make_unique<Net::DummySink>());
    req->addHeaderProxy(std::make_unique<Net::RawHeaderProxy>(QList<Net::HeaderPair>{
        { .headerName = "Authorization", .headerValue = QString("Bearer %1").arg(token).toLocal8Bit() },
    }));
    return req;
}

inline Net::NetRequest::Ptr makeCapeChangeRequest(QString token, QString capeId)
{
    auto req = Net::NetRequest::makeCustomRequest(QUrl("https://api.minecraftservices.com/minecraft/profile/capes/active"),
                                                  capeId.isEmpty()
                                                      ? Net::NetRequest::PostData{ nullptr }
                                                      : Net::NetRequest::PostData{ QString(R"({"capeId":"%1"})").arg(capeId).toUtf8() },
                                                  capeId.isEmpty() ? Net::NetRequest::HttpMethod::Delete : Net::NetRequest::HttpMethod::Put,
                                                  Net::NetRequest::Option::NoOptions, QString(), taskMCSkinsLogC);
    req->setSink(std::make_unique<Net::DummySink>());
    req->addHeaderProxy(std::make_unique<Net::RawHeaderProxy>(QList<Net::HeaderPair>{
        { .headerName = "Authorization", .headerValue = QString("Bearer %1").arg(token).toLocal8Bit() },
    }));
    return req;
}
