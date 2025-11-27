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

#include "ImgurAlbumCreation.h"

#include <net/Upload.h>

#include <QDebug>
#include <QJsonObject>
#include <QList>
#include <QNetworkRequest>
#include <QUrl>
#include <memory>

#include "BuildConfig.h"
#include "net/RawHeaderProxy.h"

auto ImgurAlbumCreation::Sink::init(Net::NetRequest*) -> Task::State
{
    m_output.clear();
    return Task::State::Running;
}

auto ImgurAlbumCreation::Sink::write(QByteArray& data) -> Task::State
{
    m_output.append(data);
    return Task::State::Running;
}

auto ImgurAlbumCreation::Sink::abort() -> Task::State
{
    m_output.clear();
    m_fail_reason = "Aborted";
    return Task::State::Failed;
}

auto ImgurAlbumCreation::Sink::finalize(Net::NetRequest*) -> Task::State
{
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(m_output, &jsonError);
    if (jsonError.error != QJsonParseError::NoError) {
        qDebug() << jsonError.errorString();
        m_fail_reason = "Invalid json reply";
        return Task::State::Failed;
    }
    auto object = doc.object();
    if (!object.value("success").toBool()) {
        qDebug() << doc.toJson();
        m_fail_reason = "Failed to create album";
        return Task::State::Failed;
    }
    m_result->deleteHash = object.value("data").toObject().value("deletehash").toString();
    m_result->id = object.value("data").toObject().value("id").toString();
    return Task::State::Succeeded;
}

Net::NetRequest::Ptr ImgurAlbumCreation::make(std::shared_ptr<Result> output, QList<ScreenShot::Ptr> screenshots)
{
    auto request = makeShared<Net::NetRequest>(QUrl(BuildConfig.IMGUR_BASE_URL + "album"), new Sink(output));

    request->addHeadersFromProxy(Net::RawHeaderProxy(
        QList<Net::HeaderPair>{ { "Content-Type", "application/x-www-form-urlencoded" },
                                { "Authorization", QString("Client-ID %1").arg(BuildConfig.IMGUR_CLIENT_ID).toUtf8() },
                                { "Accept", "application/json" } }));

    QStringList hashes;
    for (auto shot : screenshots) {
        hashes.append(shot->m_imgurDeleteHash);
    }
    const auto data = "deletehashes=" + hashes.join(',').toUtf8() + "&title=Minecraft%20Screenshots&privacy=hidden";
    Net::Upload::configureRequest(request.get(), data);

    return request;
}