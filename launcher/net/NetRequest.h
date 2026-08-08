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
#include <QTimer>
#include <QUrl>
#include <array>
#include <chrono>
#include <cstdint>
#include <utility>
#include <variant>

#include "Application.h"
#include "EnumWrapper.h"
#include "HeaderProxy.h"
#include "HttpMetaCache.h"
#include "Sink.h"
#include "Validator.h"

#include "QObjectPtr.h"
#include "net/Logging.h"
#include "tasks/Task.h"

class QIODevice;
class QHttpMultiPart;

namespace Net {
class ByteArraySink;

class NetRequest : public Task {
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<class NetRequest>;
    enum class Option : std::uint8_t {
        NoOptions = 0,
        AcceptLocalFiles = 1,
        MakeEternal = 2,
        AutoRetry = 4,
        AddAPIHeaders = 8,
    };
    Q_DECLARE_FLAGS(Options, Option)

    enum class HttpMethodValue : std::uint8_t {
        Get,
        Post,
        Put,
        Patch,
        Delete,
        Head,
        Options,
        Connect,
        Trace,
    };
    struct HttpMethod : EnumWrapper<HttpMethod, HttpMethodValue> {
        static constexpr auto invalid() { return Get; };
        static constexpr auto mapping()
        {
            return std::array{ std::pair{ Get, "GET" },         std::pair{ Post, "POST" },       std::pair{ Put, "PUT" },
                               std::pair{ Patch, "PATCH" },     std::pair{ Delete, "DELETE" },   std::pair{ Head, "HEAD" },
                               std::pair{ Options, "OPTIONS" }, std::pair{ Connect, "CONNECT" }, std::pair{ Trace, "TRACE" } };
        };
        using enum HttpMethodValue;
        using Base = EnumWrapper<HttpMethod, HttpMethodValue>;
        using Base::Base; /* inherit ctor */
    };

    using PostData = std::variant<QIODevice*, QHttpMultiPart*, QByteArray, std::nullptr_t>;
    using LogCatFunc = const QLoggingCategory& (*)();

   public:
#if defined(LAUNCHER_APPLICATION)
    static auto makeCached(const QUrl& url, MetaEntryPtr entry, Options options = Option::NoOptions) -> NetRequest::Ptr;
#endif

    /**
     * Creates a request downloading to the returned QByteArray,.
     * The QByteArray will live as long as the Download object.
     */
    static auto makeByteArray(const QUrl& url, Options options = Option::NoOptions) -> std::pair<NetRequest::Ptr, QByteArray*>;
    static auto makeByteArray(const QUrl& url, QByteArray postData, Options options = Option::NoOptions)
        -> std::pair<NetRequest::Ptr, QByteArray*>;
    static auto makeFile(const QUrl& url, const QString& path, Options options = Option::NoOptions) -> NetRequest::Ptr;
    static auto makeCustomRequest(const QUrl& url,
                                  PostData postData,
                                  HttpMethod method = HttpMethod::Post,
                                  Options options = Option::NoOptions,
                                  const QString& name = QString(),
                                  LogCatFunc logCat = taskNetLogC) -> NetRequest::Ptr;

   public:
    ~NetRequest() override = default;
    void addValidator(Validator* v);
    auto abort() -> bool override;
    auto canAbort() const -> bool override { return true; }

    void setNetwork(QNetworkAccessManager* network) { m_network = network; }
    void addHeaderProxy(std::unique_ptr<Net::HeaderProxy> proxy) { m_headerProxies.push_back(std::move(proxy)); }
    void setSink(std::unique_ptr<Sink> sink) { m_sink = std::move(sink); }

    // automatically handle HTTP 429 Too Many Requests errors and retry
    void enableAutoRetry(bool enable);

    QUrl url() const;
    void setUrl(QUrl url) { m_url = std::move(url); }
    int replyStatusCode() const;
    QNetworkReply::NetworkError error() const;
    QString errorString() const;

   private:
    auto handleRedirect() -> bool;
    void handleAutoRetry(int64_t delay);
    QNetworkReply* getReply(QNetworkRequest&);

   protected slots:
    void onProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadError(QNetworkReply::NetworkError error);
    void sslErrors(const QList<QSslError>& errors);
    void downloadFinished();
    void downloadReadyRead();
    void executeTask() override;

   protected:
    explicit NetRequest();
    explicit NetRequest(const QUrl& url, Options options = Option::NoOptions, const QString& name = QString());
    NetRequest(const QUrl& url, QByteArray postData, Options options);
    explicit NetRequest(const QUrl& url,
                        PostData postData,
                        HttpMethod method = HttpMethod::Post,
                        Options options = Option::NoOptions,
                        const QString& name = QString(),
                        LogCatFunc logCat = taskNetLogC);

    std::unique_ptr<Sink> m_sink;
    Options m_options;

    LogCatFunc m_logCat = taskNetLogC;

    std::chrono::time_point<std::chrono::steady_clock> m_lastProgressTime;
    qint64 m_lastProgressBytes = 0;

    QNetworkAccessManager* m_network = nullptr;

    /// the network reply
    std::unique_ptr<QNetworkReply> m_reply;
    QByteArray m_errorResponse;

    /// source URL
    QUrl m_url;
    std::vector<std::unique_ptr<Net::HeaderProxy>> m_headerProxies;

    int m_retryCount = 0;
    QTimer m_retryTimer;

    HttpMethod m_httpMethod = HttpMethod::Get;
    PostData m_postData{ nullptr };
};

}  // namespace Net

Q_DECLARE_OPERATORS_FOR_FLAGS(Net::NetRequest::Options)
