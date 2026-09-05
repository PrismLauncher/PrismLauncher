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

#include "Request.h"

#include <QDateTime>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QIODevice>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <cstdint>
#include <memory>
#include <utility>
#include <variant>

#if defined(LAUNCHER_APPLICATION)
#include "Application.h"
#include "config/GlobalConfig.h"
#include "net/ApiHeaderProxy.h"
#include "net/ChecksumValidator.h"
#include "net/MetaCacheSink.h"
#else
#include "BuildConfig.h"
#endif
#include "net/ByteArraySink.h"
#include "net/FileSink.h"

#include "MMCTime.h"
#include "StringUtils.h"

namespace Net {

namespace {
auto logCatForMethod(HttpMethod method) -> Request::LogCatFunc
{
    switch (method.value()) {
        case HttpMethod::Get:
            return taskDownloadLogC;
        case HttpMethod::Post:
            return taskUploadLogC;
        default:
            break;
    }
    return taskNetLogC;
}
}  // namespace

Request::Request() : Request(Spec{}) {}

Request::Request(const QUrl& url, Options options, const QString& name)
    : Request(Spec{ .method = HttpMethod::Get, .url = url, .data = std::monostate{}, .options = options, .name = name })
{}

Request::Request(const QUrl& url, QByteArray postData, Options options)
    : Request(Spec{ .method = HttpMethod::Post, .url = url, .data = std::move(postData), .options = options })
{}

Request::Request(const Spec& spec) : m_options(spec.options), m_url(spec.url), m_httpMethod(spec.method), m_postData(spec.data)
{
    connect(&m_retryTimer, &QTimer::timeout, this, &Request::executeTask);

    if (spec.name.isEmpty()) {
        setObjectName(QString("BYTES:") + m_url.toString());
    } else {
        setObjectName(spec.name);
    }
    m_logCat = logCatForMethod(m_httpMethod);
#if defined(LAUNCHER_APPLICATION)
    if (spec.options.testFlag(Option::AddAPIHeaders)) {
        addHeaderProxy(std::make_unique<ApiHeaderProxy>());
    }
#endif
}

void Request::addValidator(Validator* v)
{
    m_sink->addValidator(v);
}

void Request::executeTask()
{
    setStatus(tr("Requesting %1").arg(StringUtils::truncateUrlHumanFriendly(m_url, 80)));

    if (m_network == nullptr) {
#if defined(LAUNCHER_APPLICATION)
        m_network = APPLICATION->network();
#else
        qCCritical(m_logCat) << getUid().toString() << "No network manager set for request:" << m_url.toString();
        emit failed("No network manager set for request");
        emit finished();
        return;
#endif
    }
    if (getState() == Task::State::AbortedByUser) {
        qCWarning(m_logCat) << getUid().toString() << "Attempt to start an aborted Request:" << m_url.toString();
        emit aborted();
        emit finished();
        return;
    }

    QNetworkRequest request(m_url);
    m_state = m_sink->init(request);
    switch (m_state) {
        case State::Succeeded:
            qCDebug(m_logCat) << getUid().toString() << "Request cache hit" << m_url.toString();
            emit succeeded();
            emit finished();
            return;
        case State::Running:
            qCDebug(m_logCat) << getUid().toString() << "Running" << m_url.toString();
            break;
        case State::Inactive:
        case State::Failed:
            m_failReason = m_sink->failReason();
            emit failed(m_sink->failReason());
            emit finished();
            return;
        case State::AbortedByUser:
            emit aborted();
            emit finished();
            return;
    }

#if defined(LAUNCHER_APPLICATION)
    auto userAgent = APPLICATION->getUserAgent();
#else
    auto userAgent = BuildConfig.USER_AGENT;
#endif
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent.toUtf8());
    for (auto& headerProxy : m_headerProxies) {
        headerProxy->writeHeaders(request);
    }

#if defined(LAUNCHER_APPLICATION)
    request.setTransferTimeout(APPLICATION->config()->requestTimeout * 1000);
#else
    request.setTransferTimeout();
#endif

    m_lastProgressTime = std::chrono::steady_clock::now();
    m_lastProgressBytes = 0;

    auto* rep = getReply(request);
    if (rep == nullptr) {  // it failed
        return;
    }
    m_reply.reset(rep);
    connect(rep, &QNetworkReply::uploadProgress, this, &Request::onProgress);
    connect(rep, &QNetworkReply::downloadProgress, this, &Request::onProgress);
    connect(rep, &QNetworkReply::finished, this, &Request::downloadFinished);
    connect(rep, &QNetworkReply::errorOccurred, this, &Request::downloadError);
    connect(rep, &QNetworkReply::sslErrors, this, &Request::sslErrors);
    connect(rep, &QNetworkReply::readyRead, this, &Request::downloadReadyRead);
}

void Request::onProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - m_lastProgressTime;

    // use milliseconds for speed precision
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
    auto bytesReceivedSince = bytesReceived - m_lastProgressBytes;
    auto dlSpeedBps = static_cast<double>(bytesReceivedSince) / static_cast<double>(elapsedMs.count()) * 1000;
    auto remainingTimeS = static_cast<double>(bytesTotal - bytesReceived) / dlSpeedBps;

    //: Current amount of bytes downloaded, out of the total amount of bytes in the download
    QString dlProgress = tr("%1 / %2")
                             .arg(StringUtils::humanReadableFileSize(static_cast<double>(bytesReceived)))
                             .arg(StringUtils::humanReadableFileSize(static_cast<double>(bytesTotal)));

    QString dlSpeedStr;
    if (elapsedMs.count() > 0) {
        auto strEta = bytesTotal > 0 ? Time::humanReadableDuration(remainingTimeS) : tr("unknown");
        //: Download speed, in bytes per second (remaining download time in parenthesis)
        dlSpeedStr = tr("%1 /s (%2)").arg(StringUtils::humanReadableFileSize(dlSpeedBps)).arg(strEta);
    } else {
        //: Download speed at 0 bytes per second
        dlSpeedStr = tr("0 B/s");
    }

    setDetails(dlProgress + "\n" + dlSpeedStr);

    setProgress(bytesReceived, bytesTotal);
}

void Request::downloadError(QNetworkReply::NetworkError error)
{
    if (error == QNetworkReply::OperationCanceledError) {
        qCCritical(m_logCat) << getUid().toString() << "Aborted" << m_url.toString();
        m_state = State::Failed;
    } else if (replyStatusCode() == 429 /* HTTP Too Many Requests*/ && m_options.testFlag(Option::AutoRetry)) {
        qCDebug(m_logCat) << getUid().toString() << "Rate Limited!";
        auto delay = static_cast<int64_t>(10 * std::pow(2, m_retryCount));
        if (m_reply->hasRawHeader("Retry-After")) {
            auto retryAfter = m_reply->rawHeader("Retry-After");
            if (retryAfter.trimmed().endsWith("GMT")) /* HTTP Date format */ {
                auto afterTimestamp = QDateTime::fromString(QString::fromUtf8(retryAfter.trimmed()), "ddd, dd MMM yyyy HH:mm:ss 'GMT'");
                if (afterTimestamp.isValid()) {
                    auto seconds = QDateTime::currentDateTime().secsTo(afterTimestamp);
                    if (seconds > 0) {
                        delay = seconds;
                    }
                }
            } else {
                bool ok = false;
                auto seconds = retryAfter.toLongLong(&ok);
                if (ok && seconds > 0) {
                    delay = seconds;
                }
            }
        }
        handleAutoRetry(delay);
    } else {
        if (m_options.testFlag(Option::AcceptLocalFiles)) {
            if (m_sink->hasLocalData()) {
                m_state = State::Succeeded;
                return;
            }
        }
        // error happened during download.
        qCCritical(m_logCat) << getUid().toString() << "Failed" << m_url.toString() << "with error" << error;
        if (m_reply) {
            qCCritical(m_logCat) << getUid().toString() << "HTTP status:" << replyStatusCode() << errorString();
        }
        if (m_errorResponse.size() > 0) {
            qCCritical(m_logCat) << getUid().toString() << "Response from server:" << m_errorResponse;
        }
        m_state = State::Failed;
    }
}

void Request::sslErrors(const QList<QSslError>& errors)
{
    int i = 1;
    for (const auto& error : errors) {
        qCCritical(m_logCat).nospace() << getUid().toString() << " Request " << m_url.toString() << " SSL Error #" << i << ": "
                                       << error.errorString();
        auto cert = error.certificate();
        qCCritical(m_logCat) << getUid().toString() << "Certificate in question:\n" << cert.toText();
        i++;
    }
}

auto Request::handleRedirect() -> bool
{
    QUrl redirect = m_reply->header(QNetworkRequest::LocationHeader).toUrl();
    if (!redirect.isValid()) {
        if (!m_reply->hasRawHeader("Location")) {
            // no redirect -> it's fine to continue
            return false;
        }
        // there is a Location header, but it's not correct. we need to apply some workarounds...
        QByteArray redirectBA = m_reply->rawHeader("Location");
        if (redirectBA.size() == 0) {
            // empty, yet present redirect header? WTF?
            return false;
        }
        QString redirectStr = QString::fromUtf8(redirectBA);

        if (redirectStr.startsWith("//")) {
            /*
             * IF the URL begins with //, we need to insert the URL scheme.
             * See: https://bugreports.qt.io/browse/QTBUG-41061
             * See: http://tools.ietf.org/html/rfc3986#section-4.2
             */
            redirectStr = m_reply->url().scheme() + ":" + redirectStr;
        } else if (redirectStr.startsWith("/")) {
            /*
             * IF the URL begins with /, we need to process it as a relative URL
             */
            auto url = m_reply->url();
            url.setPath(redirectStr, QUrl::TolerantMode);
            redirectStr = url.toString();
        }

        /*
         * Next, make sure the URL is parsed in tolerant mode. Qt doesn't parse the location header in tolerant mode, which causes
         * issues.
         * FIXME: report Qt bug for this
         */
        redirect = QUrl(redirectStr, QUrl::TolerantMode);
        if (!redirect.isValid()) {
            qCWarning(m_logCat) << getUid().toString() << "Failed to parse redirect URL:" << redirectStr;
            downloadError(QNetworkReply::ProtocolFailure);
            return false;
        }
        qCDebug(m_logCat) << getUid().toString() << "Fixed location header:" << redirect;
    } else {
        qCDebug(m_logCat) << getUid().toString() << "Location header:" << redirect;
    }

    m_url = QUrl(redirect.toString());
    qCDebug(m_logCat) << getUid().toString() << "Following redirect to" << m_url.toString();
    if (++m_redirectCount > 10) {
        qCWarning(m_logCat) << getUid().toString() << "Too many redirects for" << m_url.toString();
        m_sink->abort();
        emitFailed(tr("Too many redirects"));
        return true;
    }
    executeTask();

    return true;
}

void Request::handleAutoRetry(int64_t delay)
{
    m_retryCount++;
    if (delay > 60 || m_retryCount > 4) {
        /* 1 minute is too long to wait for retry, fail for now */
        m_state = State::Failed;
        auto retryAfter = QDateTime::currentDateTime().addSecs(delay);
        emitFailed(tr("Request Rate Limited for %n second(s): Retry After %1", "seconds", static_cast<int>(delay))
                       .arg(retryAfter.toLocalTime().toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat))));
        return;
    }
    qCDebug(m_logCat) << getUid().toString() << "Retyring Request in" << delay << "seconds";
    setStatus(tr("Rate Limited: Waiting %n second(s)", "seconds", static_cast<int>(delay)));
    m_retryTimer.setTimerType(Qt::VeryCoarseTimer);
    m_retryTimer.setSingleShot(true);
    m_retryTimer.setInterval(static_cast<int>(delay) * 1000);
    m_retryTimer.start();
}

void Request::downloadFinished()
{
    // currently waiting for retry
    if (m_retryTimer.isActive()) {
        return;
    }

    // handle HTTP redirection first
    if (handleRedirect()) {
        qCDebug(m_logCat) << getUid().toString() << "Request redirected:" << m_url.toString();
        return;
    }

    // if the download failed before this point ...
    if (m_state == State::Succeeded)  // pretend to succeed so we continue processing :)
    {
        qCDebug(m_logCat) << getUid().toString() << "Request failed but we are allowed to proceed:" << m_url.toString();
        m_sink->abort();
        emit succeeded();
        emit finished();
        return;
    }
    if (m_state == State::Failed) {
        qCDebug(m_logCat) << getUid().toString() << "Request failed in previous step:" << m_url.toString();
        m_sink->abort();
        m_failReason = m_reply->errorString();
        emit failed(m_reply->errorString());
        emit finished();
        return;
    }
    if (m_state == State::AbortedByUser) {
        qCDebug(m_logCat) << getUid().toString() << "Request aborted in previous step:" << m_url.toString();
        m_sink->abort();
        emit aborted();
        emit finished();
        return;
    }

    // make sure we got all the remaining data, if any
    auto data = m_reply->readAll();
    if (!data.isEmpty()) {
        qCDebug(m_logCat) << getUid().toString() << "Writing extra" << data.size() << "bytes";
        m_state = m_sink->write(data);
        if (m_state != State::Succeeded) {
            qCDebug(m_logCat) << getUid().toString() << "Request failed to write:" << m_url.toString();
            m_sink->abort();
            m_failReason = m_sink->failReason();
            emit failed(m_sink->failReason());
            emit finished();
            return;
        }
    }

    // otherwise, finalize the whole graph
    m_state = m_sink->finalize(*m_reply);
    if (m_state != State::Succeeded) {
        qCDebug(m_logCat) << getUid().toString() << "Request failed to finalize:" << m_url.toString();
        m_sink->abort();
        m_failReason = m_sink->failReason();
        emit failed(m_sink->failReason());
        emit finished();
        return;
    }

    qCDebug(m_logCat) << getUid().toString() << "Request succeeded:" << m_url.toString();
    emit succeeded();
    emit finished();
}

void Request::downloadReadyRead()
{
    if (m_state == State::Running) {
        auto data = m_reply->readAll();
        m_state = m_sink->write(data);
        if (replyStatusCode() >= 400) {
            m_errorResponse.append(data);
        }
        if (m_state == State::Failed) {
            qCCritical(m_logCat) << getUid().toString() << "Failed to process response chunk:" << m_sink->failReason();
        }
        // qDebug() << "Request" << m_url.toString() << "gained" << data.size() << "bytes";
    } else {
        qCCritical(m_logCat) << getUid().toString() << "Cannot write download data! illegal status" << m_status;
    }
}

auto Request::abort() -> bool
{
    m_retryTimer.stop();
    m_state = State::AbortedByUser;
    if (m_reply && !m_reply->isFinished()) {
        disconnect(m_reply.get(), &QNetworkReply::errorOccurred, nullptr, nullptr);
        m_reply->abort();
    } else {
        emit aborted();
        emit finished();
    }
    return true;
}

int Request::replyStatusCode() const
{
    return m_reply ? m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() : -1;
}

QNetworkReply::NetworkError Request::error() const
{
    return m_reply ? m_reply->error() : QNetworkReply::NoError;
}

QUrl Request::url() const
{
    return m_url;
}

QString Request::errorString() const
{
    return m_reply ? m_reply->errorString() : "";
}

void Request::enableAutoRetry(bool enable)
{
    if (enable) {
        m_options |= Option::AutoRetry;
    } else {
        m_options &= ~static_cast<std::uint8_t>(Option::AutoRetry);
    }
}

QNetworkReply* Request::getReply(QNetworkRequest& request)
{
    if (m_httpMethod == HttpMethod::Get) {
        Q_ASSERT(std::holds_alternative<std::monostate>(m_postData));
        return m_network->get(request);
    }
    return std::visit(
        [this, &request](const auto& data) -> QNetworkReply* {
            using T = std::remove_cvref_t<decltype(data)>;
            const auto verb = m_httpMethod.toString().toUtf8();
            if constexpr (std::is_same_v<T, QByteArray>) {
                if (m_httpMethod == HttpMethod::Post && !request.hasRawHeader("Content-Type")) {
                    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
                }
                return m_network->sendCustomRequest(request, verb, data);
            } else if constexpr (std::is_same_v<T, std::monostate>) {
                return m_network->sendCustomRequest(request, verb);
            } else if constexpr (std::is_same_v<T, DeviceFactory>) {
                if (QIODevice* device = data(); device != nullptr) {
                    device->setParent(this);
                    return m_network->sendCustomRequest(request, verb, device);
                }
                return m_network->sendCustomRequest(request, verb);
            } else if constexpr (std::is_same_v<T, MultiPartFactory>) {
                if (QHttpMultiPart* multiPart = data(); multiPart != nullptr) {
                    if (multiPart->parent() == nullptr) {
                        multiPart->setParent(this);
                    }
                    return m_network->sendCustomRequest(request, verb, multiPart);
                }
                return m_network->sendCustomRequest(request, verb);
            }
        },
        m_postData);
}

#if defined(LAUNCHER_APPLICATION)
auto Request::makeCached(const QUrl& url, MetaEntryPtr entry, Options options) -> Ptr
{
    auto dl = Ptr(new Request(url, options, (QString("CACHE:") + url.toString())));
    auto* md5Node = new ChecksumValidator(QCryptographicHash::Md5);
    auto* cachedNode = new MetaCacheSink(std::move(entry), md5Node, options.testFlag(Option::MakeEternal));
    dl->m_sink.reset(cachedNode);
    return dl;
}
#endif

auto Request::makeByteArray(const QUrl& url, QByteArray postData, Options options) -> std::pair<Ptr, QByteArray*>
{
    auto dl = Ptr(new Request(url, std::move(postData), options));

    auto sink = std::make_unique<ByteArraySink>();
    auto* response = sink->output();
    dl->m_sink = std::move(sink);

    return { dl, response };
}

auto Request::makeByteArray(const QUrl& url, Options options) -> std::pair<Ptr, QByteArray*>
{
    auto dl = Ptr(new Request(url, options));

    auto sink = std::make_unique<ByteArraySink>();
    auto* response = sink->output();
    dl->m_sink = std::move(sink);

    return { dl, response };
}

auto Request::makeFile(const QUrl& url, const QString& path, Options options) -> Ptr
{
    auto dl = Ptr(new Request(url, options, QString("FILE:") + url.toString()));
    dl->m_sink = std::make_unique<FileSink>(path);

    return dl;
}

auto Request::makeCustomRequest(const Spec& spec) -> Ptr
{
    return Ptr(new Request(spec));
}

}  // namespace Net
