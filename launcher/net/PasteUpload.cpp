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

#include <QHttpPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrlQuery>
#include "logs/AnonymizeLog.h"

const std::array<PasteUpload::PasteTypeInfo, 4> PasteUpload::PasteTypes = { { { "0x0.st", "https://0x0.st", "" },
                                                                              { "hastebin", "https://hst.sh", "/documents" },
                                                                              { "paste.gg", "https://paste.gg", "/api/v1/pastes" },
                                                                              { "mclo.gs", "https://api.mclo.gs", "/1/log" } } };

QNetworkReply* PasteUpload::getReply(QNetworkRequest& request)
{
    switch (m_paste_type) {
        case PasteUpload::NullPointer: {
            QHttpMultiPart* multiPart = new QHttpMultiPart{ QHttpMultiPart::FormDataType, this };

            QHttpPart filePart;
            filePart.setBody(m_log.toUtf8());
            filePart.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
            filePart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"file\"; filename=\"log.txt\"");
            multiPart->append(filePart);

            return m_network->post(request, multiPart);
        }
        case PasteUpload::Hastebin: {
            return m_network->post(request, m_log.toUtf8());
        }
        case PasteUpload::Mclogs: {
            QUrlQuery postData;
            postData.addQueryItem("content", m_log);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
            return m_network->post(request, postData.toString().toUtf8());
        }
        case PasteUpload::PasteGG: {
            QJsonObject obj;
            QJsonDocument doc;
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

            obj.insert("expires", QDateTime::currentDateTimeUtc().addDays(100).toString(Qt::DateFormat::ISODate));

            QJsonArray files;
            QJsonObject logFileInfo;
            QJsonObject logFileContentInfo;
            logFileContentInfo.insert("format", "text");
            logFileContentInfo.insert("value", m_log);
            logFileInfo.insert("name", "log.txt");
            logFileInfo.insert("content", logFileContentInfo);
            files.append(logFileInfo);

            obj.insert("files", files);

            doc.setObject(obj);
            return m_network->post(request, doc.toJson());
        }
    }

    return nullptr;
};

auto PasteUpload::Sink::finalize(QNetworkReply& reply) -> Task::State
{
    if (!finalizeAllValidators(reply)) {
        m_fail_reason = "Failed to finalize validators";
        return Task::State::Failed;
    }
    int statusCode = reply.attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply.error() != QNetworkReply::NetworkError::NoError) {
        m_fail_reason = QObject::tr("Network error: %1").arg(reply.errorString());
        return Task::State::Failed;
    } else if (statusCode != 200 && statusCode != 201) {
        QString reasonPhrase = reply.attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
        m_fail_reason =
            QObject::tr("Error: %1 returned unexpected status code %2 %3").arg(m_d->url().toString()).arg(statusCode).arg(reasonPhrase);
        return Task::State::Failed;
    }

    switch (m_d->m_paste_type) {
        case PasteUpload::NullPointer:
            m_d->m_pasteLink = QString::fromUtf8(*m_output).trimmed();
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
                m_d->m_pasteLink = m_d->m_baseUrl + "/" + key;
            } else {
                qDebug() << "Log upload failed:" << doc.toJson();
                m_fail_reason = QObject::tr("Error: %1 returned a malformed response body").arg(m_d->url().toString());
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
                    m_d->m_pasteLink = obj["url"].toString();
                } else {
                    QString error = obj["error"].toString();
                    m_fail_reason = QObject::tr("Error: %1 returned an error: %2").arg(m_d->url().toString(), error);
                    return Task::State::Failed;
                }
            } else {
                qDebug() << "Log upload failed:" << doc.toJson();
                m_fail_reason = QObject::tr("Error: %1 returned a malformed response body").arg(m_d->url().toString());
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
                    m_d->m_pasteLink = m_d->m_baseUrl + "/p/anonymous/" + obj["result"].toObject()["id"].toString();
                } else {
                    QString error = obj["error"].toString();
                    QString message = (obj.contains("message") && obj["message"].isString()) ? obj["message"].toString() : "none";
                    m_fail_reason =
                        QObject::tr("Error: %1 returned an error code: %2\nError message: %3").arg(m_d->url().toString(), error, message);
                    return Task::State::Failed;
                }
            } else {
                qDebug() << "Log upload failed:" << doc.toJson();
                m_fail_reason = QObject::tr("Error: %1 returned a malformed response body").arg(m_d->url().toString());
                return Task::State::Failed;
            }
            break;
    }
    return Task::State::Succeeded;
}

PasteUpload::PasteUpload(const QString& log, QString url, PasteType pasteType) : m_log(log), m_baseUrl(url), m_paste_type(pasteType)
{
    anonymizeLog(m_log);
    auto base = PasteUpload::PasteTypes.at(pasteType);
    if (m_baseUrl.isEmpty())
        m_baseUrl = base.defaultBase;

    // HACK: Paste's docs say the standard API path is at /api/<version> but the official instance paste.gg doesn't follow that??
    if (pasteType == PasteUpload::PasteGG && m_baseUrl == base.defaultBase)
        m_url = "https://api.paste.gg/v1/pastes";
    else
        m_url = m_baseUrl + base.endpointPath;

    m_sink.reset(new Sink(this, m_output.get()));
}
