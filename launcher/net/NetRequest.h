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

#include <curl/curl.h>

#include <chrono>

#include "HeaderProxy.h"
#include "Sink.h"
#include "Validator.h"

#include "net/Logging.h"

namespace Net {
class NetRequest final : public QObject {
    Q_OBJECT

   public:
    using Ptr = shared_qobject_ptr<NetRequest>;

    enum class Option { NoOptions = 0, AcceptLocalFiles = 1, MakeEternal = 2 };
    Q_DECLARE_FLAGS(Options, Option)

    struct Multipart {
        QString name;
        QByteArray data;
        QString contentType;
        QString remoteFileName;
    };
   private:
    using logCatFunc = const QLoggingCategory& (*)();

   public:
    explicit NetRequest(const QUrl& url, Sink* sink, Options options = Option::NoOptions);

   public:
    ~NetRequest() override;

    void setLoggingCategory(logCatFunc loggingCategory);
    TaskStepProgress stepProgress();
    void updateDetails();

    QUrl url() const;
    void setUrl(QUrl url);

    CURL* curlHandle() const;

    void addValidator(Validator* v) const;

    Task::State prepare();
    void abort();
    void finalize();

    bool isSuccess() const;
    long responseCode() const;
    CURLcode result() const;
    void setResult(CURLcode result);
    QString error() const;

    bool hasHeader(const char* name) const;
    QString getHeader(const char* name) const;
    void addHeader(const char* name, const char* value);
    void addHeadersFromProxy(const HeaderProxy& proxy);

    void httpPut(QByteArray data);
    void httpPost(const char* contentType, QByteArray data);
    void httpMultipart(QList<Multipart> parts);
    void httpDelete() const;

    void setTotalBytes(qint64 totalBytes);

   signals:
    void finished();
    void succeeded();
    void failed(QString reason);
    void aborted();

   private:
    static size_t curlReadCallback(char* buffer, size_t, size_t bufferSize, void* thisRequest);
    static size_t curlWriteCallback(const char* data, size_t, size_t dataSize, void* thisRequest);
    static size_t curlProgressCallback(void* thisRequest,
                                curl_off_t downloadBytesExpected,
                                curl_off_t downloadBytesReceived,
                                curl_off_t uploadBytesExpected,
                                curl_off_t uploadBytesReceived);
    static void curlFreeSlist(curl_slist* ptr);
    static void curlFreeMime(curl_mime* ptr);

   private:
    logCatFunc m_logCat = taskNetLogC;
    TaskStepProgress m_stepProgress{};

    QUrl m_url{};
    std::unique_ptr<Sink> m_sink{nullptr};
    Options m_options{};

    QByteArray m_uploadData{};
    size_t m_uploadDataOffset = 0;

    std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> m_curlHeaders{ nullptr, curlFreeSlist };
    std::unique_ptr<curl_mime, decltype(&curl_mime_free)> m_mime{ nullptr, curlFreeMime };
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> m_curl{ curl_easy_init(), curl_easy_cleanup };
    CURLcode m_result = CURLE_FAILED_INIT;
    char m_errorBuffer[CURL_ERROR_SIZE];

    std::unordered_map<std::string, std::string> m_headers{};
};
}  // namespace Net

Q_DECLARE_OPERATORS_FOR_FLAGS(Net::NetRequest::Options)
