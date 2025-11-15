// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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

#include "NetRequest.h"

#include <QDateTime>
#include <QFileInfo>
#include <QNetworkReply>
#include <QUrl>
#include <memory>

#if defined(LAUNCHER_APPLICATION)
#include "Application.h"
#endif
#include <MMCTime.h>

#include "BuildConfig.h"

#include "StringUtils.h"

namespace Net {
NetRequest::NetRequest(const QUrl& url, Sink* sink, const Options options) : m_url(url), m_sink(sink), m_options(options)
{
    if (!m_curl) {
        qCritical() << "Failed to create curl easy handle";
        throw std::bad_alloc{};
    }

    curl_easy_setopt(m_curl.get(), CURLOPT_URL, url.toString(QUrl::FullyEncoded).toStdString().c_str());
    curl_easy_setopt(m_curl.get(), CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(m_curl.get(), CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(m_curl.get(), CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(m_curl.get(), CURLOPT_XFERINFOFUNCTION, curlProgressCallback);
    curl_easy_setopt(m_curl.get(), CURLOPT_READFUNCTION, curlReadCallback);
    curl_easy_setopt(m_curl.get(), CURLOPT_WRITEFUNCTION, curlWriteCallback);

    const auto thisPtr = static_cast<void*>(this);
    curl_easy_setopt(m_curl.get(), CURLOPT_XFERINFODATA, thisPtr);
    curl_easy_setopt(m_curl.get(), CURLOPT_READDATA, thisPtr);
    curl_easy_setopt(m_curl.get(), CURLOPT_WRITEDATA, thisPtr);

    curl_easy_setopt(m_curl.get(), CURLOPT_ERRORBUFFER, m_errorBuffer);

#if defined(LAUNCHER_APPLICATION)
    const auto userAgent = APPLICATION->getUserAgent();
#else
    const auto userAgent = BuildConfig.USER_AGENT;
#endif
    curl_easy_setopt(m_curl.get(), CURLOPT_USERAGENT, userAgent.toStdString().c_str());

#if defined(LAUNCHER_APPLICATION)
    const long timeout = APPLICATION->settings()->get("RequestTimeout").toInt() * 1000;
#else
    const long timeout = 30000;
#endif
    curl_easy_setopt(m_curl.get(), CURLOPT_TIMEOUT_MS, timeout);

    connect(this, &NetRequest::succeeded, this, &NetRequest::finished);
    connect(this, &NetRequest::failed, this, &NetRequest::finished);
    connect(this, &NetRequest::aborted, this, &NetRequest::finished);
}

NetRequest::~NetRequest() = default;

void NetRequest::setLoggingCategory(logCatFunc loggingCategory)
{
    m_logCat = loggingCategory;
}

TaskStepProgress NetRequest::stepProgress()
{
    return m_stepProgress;
}

void NetRequest::updateDetails()
{
    curl_off_t downloadSpeed;
    curl_off_t uploadSpeed;
    curl_easy_getinfo(m_curl.get(), CURLINFO_SPEED_DOWNLOAD_T, &downloadSpeed);
    curl_easy_getinfo(m_curl.get(), CURLINFO_SPEED_UPLOAD_T, &uploadSpeed);

    const curl_off_t speed = std::max(downloadSpeed, uploadSpeed);
    const qint64 received = m_stepProgress.current;
    const qint64 total = m_stepProgress.total;

    const QString receivedAndTotal = total <= 0 ? "" : tr("%1 / %2").arg(StringUtils::humanReadableFileSize(received), StringUtils::humanReadableFileSize(total));
    const QString speedAndETA = speed <= 0 ? "" : tr("%1/s (%2 left)").arg(StringUtils::humanReadableFileSize(speed), Time::humanReadableDuration((total - received) / speed));
    m_stepProgress.details = receivedAndTotal + "\n" + speedAndETA;
}

QUrl NetRequest::url() const
{
    return m_url;
}

CURL* NetRequest::curlHandle() const
{
    return m_curl.get();
}

void NetRequest::addValidator(Validator* v) const
{
    m_sink->addValidator(v);
}

Task::State NetRequest::prepare()
{
    curl_slist* headers = nullptr;
    for (const auto& [name, value] : m_headers) {
        const auto combined = std::string(name) + ": " + std::string(value);

        const auto newHeaders = curl_slist_append(headers, combined.c_str());
        if (!newHeaders) {
            qCCritical(m_logCat).nospace() << "Failed to append header: " << combined;
            continue;
        }

        headers = newHeaders;
    }

    curl_easy_setopt(m_curl.get(), CURLOPT_HTTPHEADER, headers);
    m_curlHeaders.reset(headers);

    const QString templ = m_uploadData.isEmpty() ? tr("Downloading %1") : tr("Uploading %1");
    m_stepProgress.status = templ.arg(StringUtils::truncateUrlHumanFriendly(m_url, 80));

    const auto state = m_sink->init(this);
    switch (state) {
        case Task::State::Succeeded:
            qCDebug(m_logCat) << "Request" << m_url << "hit cache";
            emit succeeded();
            break;
        case Task::State::Running:
            qCDebug(m_logCat) << "Request" << m_url << "is now running";
            break;
        case Task::State::Inactive:
        case Task::State::Failed:
            qCWarning(m_logCat) << "Request" << m_url << "has failed before starting:" << m_sink->failReason();
            emit failed(m_sink->failReason());
            break;
        case Task::State::AbortedByUser:
            qCInfo(m_logCat) << "Request" << m_url << "was aborted by user before starting";
            emit aborted();
            break;
        default:
            break;
    }

    return state;
}

void NetRequest::abort()
{
    m_sink->abort();
    emit aborted();
}

void NetRequest::finalize()
{
    switch (m_sink->finalize(this)) {
        case Task::State::Succeeded:
            emit succeeded();
            break;
        case Task::State::Failed:
            emit failed(m_sink->failReason());
            break;
        case Task::State::AbortedByUser:
            emit aborted();
            break;
        default:
            break;
    }
}

bool NetRequest::isSuccess() const
{
    return m_result == CURLE_OK;
}

long NetRequest::responseCode() const
{
    long responseCode;
    curl_easy_getinfo(m_curl.get(), CURLINFO_RESPONSE_CODE, &responseCode);
    return responseCode;
}

CURLcode NetRequest::result() const
{
    return m_result;
}

void NetRequest::setResult(const CURLcode result)
{
    m_result = result;
    m_stepProgress.state = isSuccess() ? TaskStepState::Succeeded : TaskStepState::Failed;
}

QString NetRequest::error() const
{
    return m_errorBuffer[0] ? m_errorBuffer : curl_easy_strerror(m_result);
}

bool NetRequest::hasHeader(const char* name) const
{
    if (const CURLHcode result = curl_easy_header(m_curl.get(), name, 0, CURLH_HEADER, -1, nullptr); result == CURLHE_OK) {
        return true;
    }

    return m_headers.contains(name);
}

QString NetRequest::getHeader(const char* name) const
{
    curl_header* header;
    const CURLHcode result = curl_easy_header(m_curl.get(), name, 0, CURLH_HEADER, -1, &header);
    if (result == CURLHE_OK) {
        return header->value;
    }

    if (const auto stdName = std::string(name); m_headers.contains(stdName)) {
        return m_headers.at(stdName).data();
    }

    qCritical() << "Failed to get header:" << result;
    return "";
}

void NetRequest::addHeader(const char* name, const char* value)
{
    m_headers[name] = std::string(value);
}

void NetRequest::addHeadersFromProxy(const HeaderProxy& proxy)
{
    for (auto& [headerName, headerValue] : proxy.headers(m_url)) {
        addHeader(headerName, headerValue);
    }
}

void NetRequest::httpPut(QByteArray data)
{
    m_uploadData = data;
    curl_easy_setopt(m_curl.get(), CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(m_curl.get(), CURLOPT_INFILESIZE_LARGE, data.size());
}

void NetRequest::httpPost(const char* contentType, QByteArray data)
{
    if (!hasHeader("Content-Type")) {
        addHeader("Content-Type", contentType);
    }

    m_uploadData = QByteArray(data);
    curl_easy_setopt(m_curl.get(), CURLOPT_POSTFIELDS, m_uploadData.data());
    curl_easy_setopt(m_curl.get(), CURLOPT_POSTFIELDSIZE, m_uploadData.size());
}

void NetRequest::httpMultipart(QList<Multipart> parts)
{
    m_mime.reset(curl_mime_init(m_curl.get()));

    for (auto& [name, data, contentType, remoteFileName] : parts) {
        curl_mimepart* curlPart = curl_mime_addpart(m_mime.get());
        curl_mime_name(curlPart, name.toStdString().c_str());
        curl_mime_data(curlPart, data, data.size());
        if (!contentType.isEmpty()) {
            curl_mime_type(curlPart, contentType.toStdString().c_str());
        }
        if (!remoteFileName.isEmpty()) {
            curl_mime_filename(curlPart, remoteFileName.toStdString().c_str());
        }
    }

    curl_easy_setopt(m_curl.get(), CURLOPT_MIMEPOST, m_mime.get());
}

void NetRequest::httpDelete() const
{
    curl_easy_setopt(m_curl.get(), CURLOPT_CUSTOMREQUEST, "DELETE");
}

void NetRequest::setTotalBytes(const qint64 totalBytes)
{
    m_stepProgress.old_total = m_stepProgress.total;
    m_stepProgress.total = totalBytes;
}

void NetRequest::setUrl(QUrl url)
{
    m_url = url;
    curl_easy_setopt(m_curl.get(), CURLOPT_URL, url.toString(QUrl::FullyEncoded).toStdString().c_str());
}

size_t NetRequest::curlReadCallback(char* buffer, size_t, size_t bufferSize, void* thisRequest)
{
    const auto request = static_cast<NetRequest*>(thisRequest);
    const size_t bytesToWrite = std::min(bufferSize, static_cast<size_t>(request->m_uploadData.size()) - request->m_uploadDataOffset);

    std::memcpy(buffer, request->m_uploadData.data() + request->m_uploadDataOffset, bytesToWrite);
    request->m_uploadDataOffset += bytesToWrite;

    return bytesToWrite;
}

size_t NetRequest::curlWriteCallback(const char* data, size_t, const size_t dataSize, void* thisRequest)
{
    const auto request = static_cast<NetRequest*>(thisRequest);
    QByteArray qData(data, dataSize);
    request->m_sink->write(qData);

    return dataSize;
}

size_t NetRequest::curlProgressCallback(void* thisRequest,
                                        const curl_off_t downloadBytesExpected,
                                        const curl_off_t downloadBytesReceived,
                                        const curl_off_t uploadBytesExpected,
                                        const curl_off_t uploadBytesReceived)
{
    const auto request = static_cast<NetRequest*>(thisRequest);

    const curl_off_t presetTotal = request->m_stepProgress.total;
    const curl_off_t bytesExpected = std::max(presetTotal, std::max(downloadBytesExpected, uploadBytesExpected));
    const curl_off_t bytesReceived = std::max(downloadBytesReceived, uploadBytesReceived);

    request->m_stepProgress.update(bytesReceived, bytesExpected <= 0 ? -1 : bytesExpected);

    return 0;
}

void NetRequest::curlFreeSlist(curl_slist* ptr)
{
    if (ptr) {
        curl_slist_free_all(ptr);
    }
}

void NetRequest::curlFreeMime(curl_mime* ptr)
{
    if (ptr) {
        curl_mime_free(ptr);
    }
}
}  // namespace Net
