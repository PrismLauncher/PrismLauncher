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

#include <QDateTime>
#include <QDebug>
#include <QHttpPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrlQuery>
#include "logs/AnonymizeLog.h"
#include "net/RPCSink.h"
#include "net/RawHeaderProxy.h"

namespace PasteUpload {
namespace {
struct PasteTypeInfo {
    QString name;
    QString defaultBase;
    QString endpointPath;
};

const std::array<PasteTypeInfo, 4> g_PasteTypes = {
    { { .name = "0x0.st", .defaultBase = "https://0x0.st", .endpointPath = "" },
      { .name = "hastebin", .defaultBase = "https://hst.sh", .endpointPath = "/documents" },
      { .name = "paste.gg", .defaultBase = "https://paste.gg", .endpointPath = "/api/v1/pastes" },
      { .name = "mclo.gs", .defaultBase = "https://api.mclo.gs", .endpointPath = "/1/log" } }
};
}  // namespace

QString Type::defaultBase() const
{
    return g_PasteTypes.at(static_cast<std::size_t>(toInt())).defaultBase;
}

QString Type::endpointPath() const
{
    return g_PasteTypes.at(static_cast<std::size_t>(toInt())).endpointPath;
}

std::pair<Net::Request::Ptr, QString*> Type::make(QString log, QString baseUrl) const
{
    anonymizeLog(log);

    if (baseUrl.isEmpty()) {
        baseUrl = defaultBase();
    }

    QUrl url;
    // HACK: Paste's docs say the standard API path is at /api/<version> but the official instance paste.gg doesn't follow that??
    if (value() == PasteUpload::Type::PasteGG && baseUrl == defaultBase()) {
        url = "https://api.paste.gg/v1/pastes";
    } else {
        url = baseUrl + endpointPath();
    }

    Net::Request::PostData postData;
    switch (value()) {
        case PasteUpload::Type::NullPointer: {
            postData = [log = std::move(log)] {
                auto* multiPart = new QHttpMultiPart{ QHttpMultiPart::FormDataType };

                QHttpPart filePart;
                filePart.setBody(log.toUtf8());
                filePart.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
                filePart.setHeader(QNetworkRequest::ContentDispositionHeader, R"(form-data; name="file"; filename="log.txt")");
                multiPart->append(filePart);
                return multiPart;
            };
            break;
        }
        case PasteUpload::Type::Hastebin: {
            postData = log.toUtf8();
            break;
        }
        case PasteUpload::Type::Mclogs: {
            QUrlQuery postDataQuery;
            postDataQuery.addQueryItem("content", log);
            postData = postDataQuery.toString().toUtf8();
            break;
        }
        case PasteUpload::Type::PasteGG: {
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
            postData = doc.toJson();
            break;
        }
        case PasteUpload::Type::Invalid:
            break;
    }

    auto parseFunc = [baseUrl, url, this](const QByteArray& response) -> Net::RPC::Sink<QString>::ParseResult {
        switch (value()) {
            case Type::NullPointer:
                return QString::fromUtf8(response).trimmed();
            case Type::Hastebin: {
                QJsonParseError jsonError;
                auto doc = QJsonDocument::fromJson(response, &jsonError);
                if (jsonError.error != QJsonParseError::NoError) {
                    qDebug() << "hastebin server did not reply with JSON" << jsonError.errorString();
                    return std::unexpected(
                        QObject::tr("Failed to parse response from hastebin server: expected JSON but got an invalid response. Error: %1")
                            .arg(jsonError.errorString()));
                }
                auto obj = doc.object();
                if (obj.contains("key") && obj["key"].isString()) {
                    return baseUrl + "/" + obj["key"].toString();
                }
                qDebug() << "Log upload failed:" << doc.toJson();
                return std::unexpected(QObject::tr("Error: %1 returned a malformed response body").arg(url.toString()));
            }
            case Type::Mclogs: {
                QJsonParseError jsonError;
                auto doc = QJsonDocument::fromJson(response, &jsonError);
                if (jsonError.error != QJsonParseError::NoError) {
                    qDebug() << "mclogs server did not reply with JSON" << jsonError.errorString();
                    return std::unexpected(
                        QObject::tr("Failed to parse response from mclogs server: expected JSON but got an invalid response. Error: %1")
                            .arg(jsonError.errorString()));
                }
                auto obj = doc.object();
                if (obj.contains("success") && obj["success"].isBool()) {
                    bool success = obj["success"].toBool();
                    if (success) {
                        return obj["url"].toString();
                    }
                    QString error = obj["error"].toString();
                    return std::unexpected(QObject::tr("Error: %1 returned an error: %2").arg(url.toString(), error));
                }
                qDebug() << "Log upload failed:" << doc.toJson();
                return std::unexpected(QObject::tr("Error: %1 returned a malformed response body").arg(url.toString()));
            }
            case Type::PasteGG: {
                QJsonParseError jsonError;
                auto doc = QJsonDocument::fromJson(response, &jsonError);
                if (jsonError.error != QJsonParseError::NoError) {
                    qDebug() << "pastegg server did not reply with JSON" << jsonError.errorString();
                    return std::unexpected(
                        QObject::tr("Failed to parse response from pasteGG server: expected JSON but got an invalid response. Error: %1")
                            .arg(jsonError.errorString()));
                }
                auto obj = doc.object();
                if (obj.contains("status") && obj["status"].isString()) {
                    QString status = obj["status"].toString();
                    if (status == "success") {
                        return baseUrl + "/p/anonymous/" + obj["result"].toObject()["id"].toString();
                    }
                    QString error = obj["error"].toString();
                    QString message = (obj.contains("message") && obj["message"].isString()) ? obj["message"].toString() : "none";
                    return std::unexpected(
                        QObject::tr("Error: %1 returned an error code: %2\nError message: %3").arg(url.toString(), error, message));
                }
                qDebug() << "Log upload failed:" << doc.toJson();
                return std::unexpected(QObject::tr("Error: %1 returned a malformed response body").arg(url.toString()));
            }
            case Type::Invalid:
                break;
        }
        return std::unexpected(QObject::tr("Unknown paste type"));
    };

    auto dl = Net::Request::makeCustomRequest({ .method = Net::HttpMethod::Post, .url = url, .data = std::move(postData) });

    auto sink = std::make_unique<Net::RPC::Sink<QString>>(std::move(parseFunc));
    QString* pasteLink = sink->result();
    dl->setSink(std::move(sink));

    QList<Net::HeaderPair> headers;
    switch (value()) {
        case PasteUpload::Type::NullPointer:
            break;
        case PasteUpload::Type::Hastebin:
            headers = { { .headerName = "Content-Type", .headerValue = "text/plain" } };
            break;
        case PasteUpload::Type::Mclogs:
            headers = { { .headerName = "Content-Type", .headerValue = "application/x-www-form-urlencoded" } };
            break;
        case PasteUpload::Type::PasteGG:
            headers = { { .headerName = "Content-Type", .headerValue = "application/json" } };
            break;
        case PasteUpload::Type::Invalid:
            break;
    }
    if (!headers.isEmpty()) {
        dl->addHeaderProxy(std::make_unique<Net::RawHeaderProxy>(std::move(headers)));
    }

    return { dl, pasteLink };
}
}  // namespace PasteUpload
