// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
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

#pragma once

#include <QNetworkReply>
#include <QUrl>
#include <QTimer>
#include <chrono>

#include "HeaderProxy.h"
#include "Sink.h"
#include "Validator.h"

#include "QObjectPtr.h"
#include "net/Logging.h"
#include "tasks/Task.h"

namespace Net {
class NetRequest : public Task {
    Q_OBJECT
   protected:
    explicit NetRequest();

   public:
    using Ptr = shared_qobject_ptr<class NetRequest>;
    enum class Option { NoOptions = 0, AcceptLocalFiles = 1, MakeEternal = 2, AutoRetry = 4 };
    Q_DECLARE_FLAGS(Options, Option)

   public:
    ~NetRequest() override = default;
    void addValidator(Validator* v);
    auto abort() -> bool override;
    auto canAbort() const -> bool override { return true; }

    void setNetwork(QNetworkAccessManager* network) { m_network = network; }
    void addHeaderProxy(std::unique_ptr<Net::HeaderProxy> proxy) { m_headerProxies.push_back(std::move(proxy)); }

    // automatically handle HTTP 429 Too Many Requests errors and retry
    void enableAutoRetry(bool enable);

    QUrl url() const;
    void setUrl(QUrl url) { m_url = url; }
    int replyStatusCode() const;
    QNetworkReply::NetworkError error() const;
    QString errorString() const;

   private:
    auto handleRedirect() -> bool;
    void handleAutoRetry(int64_t delay);
    virtual QNetworkReply* getReply(QNetworkRequest&) = 0;

   protected slots:
    void onProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadError(QNetworkReply::NetworkError error);
    void sslErrors(const QList<QSslError>& errors);
    void downloadFinished();
    void downloadReadyRead();
    void executeTask() override;

   protected:
    std::unique_ptr<Sink> m_sink;
    Options m_options;

    using logCatFunc = const QLoggingCategory& (*)();
    logCatFunc logCat = taskUploadLogC;

    std::chrono::steady_clock m_clock;
    std::chrono::time_point<std::chrono::steady_clock> m_last_progress_time;
    qint64 m_last_progress_bytes;

    QNetworkAccessManager* m_network;

    /// the network reply
    std::unique_ptr<QNetworkReply> m_reply;
    QByteArray m_errorResponse;

    /// source URL
    QUrl m_url;
    std::vector<std::unique_ptr<Net::HeaderProxy>> m_headerProxies;

    int m_retryCount = 0;
    QTimer m_retryTimer;
};
}  // namespace Net

Q_DECLARE_OPERATORS_FOR_FLAGS(Net::NetRequest::Options)
