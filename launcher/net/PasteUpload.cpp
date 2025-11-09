// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Lenny McLennington <lenny@sneed.church>
 *  Copyright (C) 2022 Swirl <swurl@swurl.xyz>
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

#include "PasteUpload.h"
#include <qobject.h>

#include <QHttpPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

#include "Upload.h"
#include "logs/AnonymizeLog.h"

auto PasteUpload::Sink::finalize(Net::NetRequest* request) -> Task::State
{
    if (!finalizeAllValidators()) {
        m_fail_reason = "Failed to finalize validators";
        return Task::State::Failed;
    }
    int statusCode = request->responseCode();

    if (!request->isSuccess()) {
        m_fail_reason = QObject::tr("Network error: %1").arg(request->error());
        return Task::State::Failed;
    } else if (statusCode != 200 && statusCode != 201) {
        m_fail_reason =
            QObject::tr("Error: %1 returned unexpected status code %2").arg(request->url().toString()).arg(statusCode);
        return Task::State::Failed;
    }

    switch (m_meta->pasteType) {
        case PasteUpload::NullPointer:
            m_meta->pasteUrl = QString::fromUtf8(*m_output).trimmed();
            break;
        case PasteUpload::Hastebin: {
            QJsonParseError jsonError;
            auto doc = QJsonDocument::fromJson(*m_output, &jsonError);
            if (jsonError.error != QJsonParseError::NoError) {
                qDebug() << "hastebin server did not reply with JSON" << jsonError.errorString();
                m_fail_reason =
                    QObject::tr("Failed to parse response from hastebin server: expected JSON but got an invalid response. Error: %1")
                        .arg(jsonError.errorString());
                return Task::State::Failed;
            }
            auto obj = doc.object();
            if (obj.contains("key") && obj["key"].isString()) {
                QString key = doc.object()["key"].toString();
                m_meta->pasteUrl = m_meta->baseUrl + "/" + key;
            } else {
                qDebug() << "Log upload failed:" << doc.toJson();
                m_fail_reason = QObject::tr("Error: %1 returned a malformed response body").arg(request->url().toString());
                return Task::State::Failed;
            }
            break;
        }
        case PasteUpload::Mclogs: {
            QJsonParseError jsonError;
            auto doc = QJsonDocument::fromJson(*m_output, &jsonError);
            if (jsonError.error != QJsonParseError::NoError) {
                qDebug() << "mclogs server did not reply with JSON" << jsonError.errorString();
                m_fail_reason =
                    QObject::tr("Failed to parse response from mclogs server: expected JSON but got an invalid response. Error: %1")
                        .arg(jsonError.errorString());
                return Task::State::Failed;
            }
            auto obj = doc.object();
            if (obj.contains("success") && obj["success"].isBool()) {
                bool success = obj["success"].toBool();
                if (success) {
                    m_meta->pasteUrl = obj["url"].toString();
                } else {
                    QString error = obj["error"].toString();
                    m_fail_reason = QObject::tr("Error: %1 returned an error: %2").arg(request->url().toString(), error);
                    return Task::State::Failed;
                }
            } else {
                qDebug() << "Log upload failed:" << doc.toJson();
                m_fail_reason = QObject::tr("Error: %1 returned a malformed response body").arg(request->url().toString());
                return Task::State::Failed;
            }
            break;
        }
        case PasteUpload::PasteGG:
            QJsonParseError jsonError;
            auto doc = QJsonDocument::fromJson(*m_output, &jsonError);
            if (jsonError.error != QJsonParseError::NoError) {
                qDebug() << "pastegg server did not reply with JSON" << jsonError.errorString();
                m_fail_reason =
                    QObject::tr("Failed to parse response from pasteGG server: expected JSON but got an invalid response. Error: %1")
                        .arg(jsonError.errorString());
                return Task::State::Failed;
            }
            auto obj = doc.object();
            if (obj.contains("status") && obj["status"].isString()) {
                QString status = obj["status"].toString();
                if (status == "success") {
                    m_meta->pasteUrl = m_meta->baseUrl + "/p/anonymous/" + obj["result"].toObject()["id"].toString();
                } else {
                    QString error = obj["error"].toString();
                    QString message = (obj.contains("message") && obj["message"].isString()) ? obj["message"].toString() : "none";
                    m_fail_reason =
                        QObject::tr("Error: %1 returned an error code: %2\nError message: %3").arg(request->url().toString(), error, message);
                    return Task::State::Failed;
                }
            } else {
                qDebug() << "Log upload failed:" << doc.toJson();
                m_fail_reason = QObject::tr("Error: %1 returned a malformed response body").arg(request->url().toString());
                return Task::State::Failed;
            }
            break;
    }
    return Task::State::Succeeded;
}

Net::NetRequest::Ptr PasteUpload::make(QString log, std::shared_ptr<PasteMeta> meta)
{
    auto base = PasteTypes.at(meta->pasteType);
    if (meta->baseUrl.isEmpty()) {
        meta->baseUrl = base.defaultBase;
    }

    // HACK: Paste's docs say the standard API path is at /api/<version> but the official instance paste.gg doesn't follow that??
    if (meta->pasteType == PasteUpload::PasteGG && meta->baseUrl == base.defaultBase) {
        meta->baseUrl = "https://api.paste.gg/v1/pastes";
    } else {
        meta->baseUrl = meta->baseUrl + base.endpointPath;
    }

    auto request = makeShared<Net::NetRequest>(QUrl(meta->baseUrl), new Sink(meta));
    configureRequest(request.get(), log, meta);
    return request;
}

void PasteUpload::configureRequest(Net::NetRequest* request, QString log, std::shared_ptr<PasteMeta> meta)
{
    anonymizeLog(log);

    switch (meta->pasteType) {
        case PasteUpload::NullPointer: {
            const QList<Net::NetRequest::Multipart> parts = { { "file", log.toUtf8(), "text/plain", "log.txt" } };
            request->httpMultipart(parts);
            break;
        }
        case PasteUpload::Hastebin: {
            request->httpPost("text/plain", log.toUtf8());
            break;
        }
        case PasteUpload::Mclogs: {
            QUrlQuery postData;
            postData.addQueryItem("content", log);
            request->httpPost("application/x-www-form-urlencoded", postData.toString().toUtf8());
            break;
        }
        case PasteUpload::PasteGG: {
            QJsonObject obj;
            QJsonDocument doc;

            obj.insert("expires", QDateTime::currentDateTimeUtc().addDays(100).toString(Qt::DateFormat::ISODate));

            QJsonArray files;
            QJsonObject logFileInfo;
            QJsonObject logFileContentInfo;
            logFileContentInfo.insert("format", "text");
            logFileContentInfo.insert("value", log);
            logFileInfo.insert("name", "log.txt");
            logFileInfo.insert("content", logFileContentInfo);
            files.append(logFileInfo);

            obj.insert("files", files);

            doc.setObject(obj);

            Net::Upload::configureRequest(request, doc.toJson());
            break;
        }
    }

    request->setLoggingCategory(taskUploadLogC);
}