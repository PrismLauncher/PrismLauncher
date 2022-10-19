// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
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

#include "Download.h"

#include <QDateTime>
#include <QFileInfo>

#include "ByteArraySink.h"
#include "ChecksumValidator.h"
#include "FileSystem.h"
#include "MetaCacheSink.h"

#include "BuildConfig.h"
#include "Application.h"

namespace Net {

Download::Download() : NetAction()
{
    m_state = State::Inactive;
}

auto Download::makeCached(QUrl url, MetaEntryPtr entry, Options options) -> Download::Ptr
{
    auto* dl = new Download();
    dl->m_url = url;
    dl->m_options = options;
    auto md5Node = new ChecksumValidator(QCryptographicHash::Md5);
    auto cachedNode = new MetaCacheSink(entry, md5Node, options.testFlag(Option::MakeEternal));
    dl->m_sink.reset(cachedNode);
    return dl;
}

auto Download::makeByteArray(QUrl url, QByteArray* output, Options options) -> Download::Ptr
{
    auto* dl = new Download();
    dl->m_url = url;
    dl->m_options = options;
    dl->m_sink.reset(new ByteArraySink(output));
    return dl;
}

auto Download::makeFile(QUrl url, QString path, Options options) -> Download::Ptr
{
    auto* dl = new Download();
    dl->m_url = url;
    dl->m_options = options;
    dl->m_sink.reset(new FileSink(path));
    return dl;
}

void Download::addValidator(Validator* v)
{
    m_sink->addValidator(v);
}

void Download::executeTask()
{
    setStatus(tr("Downloading %1").arg(m_url.toString()));

    if (getState() == Task::State::AbortedByUser) {
        qCWarning(LAUNCHER_LOG) << "Attempt to start an aborted Download:" << m_url.toString();
        emitAborted();
        return;
    }

    QNetworkRequest request(m_url);
    m_state = m_sink->init(request);
    switch (m_state) {
        case State::Succeeded:
            emit succeeded();
            qCDebug(LAUNCHER_LOG) << "Download cache hit " << m_url.toString();
            return;
        case State::Running:
            qCDebug(LAUNCHER_LOG) << "Downloading " << m_url.toString();
            break;
        case State::Inactive:
        case State::Failed:
            emitFailed();
            return;
        case State::AbortedByUser:
            emitAborted();
            return;
    }

    request.setHeader(QNetworkRequest::UserAgentHeader, APPLICATION->getUserAgent().toUtf8());
    if (APPLICATION->capabilities() & Application::SupportsFlame
            && request.url().host().contains("api.curseforge.com")) {
        request.setRawHeader("x-api-key", APPLICATION->getFlameAPIKey().toUtf8());
    };

    QNetworkReply* rep = m_network->get(request);

    m_reply.reset(rep);
    connect(rep, &QNetworkReply::downloadProgress, this, &Download::downloadProgress);
    connect(rep, &QNetworkReply::finished, this, &Download::downloadFinished);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(rep, SIGNAL(errorOccurred(QNetworkReply::NetworkError)), SLOT(downloadError(QNetworkReply::NetworkError)));
#else
    connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(downloadError(QNetworkReply::NetworkError)));
#endif
    connect(rep, &QNetworkReply::sslErrors, this, &Download::sslErrors);
    connect(rep, &QNetworkReply::readyRead, this, &Download::downloadReadyRead);
}

void Download::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    setProgress(bytesReceived, bytesTotal);
}

void Download::downloadError(QNetworkReply::NetworkError error)
{
    if (error == QNetworkReply::OperationCanceledError) {
        qCCritical(LAUNCHER_LOG) << "Aborted " << m_url.toString();
        m_state = State::AbortedByUser;
    } else {
        if (m_options & Option::AcceptLocalFiles) {
            if (m_sink->hasLocalData()) {
                m_state = State::Succeeded;
                return;
            }
        }
        // error happened during download.
        qCCritical(LAUNCHER_LOG) << "Failed " << m_url.toString() << " with reason " << error;
        m_state = State::Failed;
    }
}

void Download::sslErrors(const QList<QSslError>& errors)
{
    int i = 1;
    for (auto error : errors) {
        qCCritical(LAUNCHER_LOG) << "Download" << m_url.toString() << "SSL Error #" << i << " : " << error.errorString();
        auto cert = error.certificate();
        qCCritical(LAUNCHER_LOG) << "Certificate in question:\n" << cert.toText();
        i++;
    }
}

auto Download::handleRedirect() -> bool
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
         * Next, make sure the URL is parsed in tolerant mode. Qt doesn't parse the location header in tolerant mode, which causes issues.
         * FIXME: report Qt bug for this
         */
        redirect = QUrl(redirectStr, QUrl::TolerantMode);
        if (!redirect.isValid()) {
            qCWarning(LAUNCHER_LOG) << "Failed to parse redirect URL:" << redirectStr;
            downloadError(QNetworkReply::ProtocolFailure);
            return false;
        }
        qCDebug(LAUNCHER_LOG) << "Fixed location header:" << redirect;
    } else {
        qCDebug(LAUNCHER_LOG) << "Location header:" << redirect;
    }

    m_url = QUrl(redirect.toString());
    qCDebug(LAUNCHER_LOG) << "Following redirect to " << m_url.toString();
    startAction(m_network);

    return true;
}

void Download::downloadFinished()
{
    // handle HTTP redirection first
    if (handleRedirect()) {
        qCDebug(LAUNCHER_LOG) << "Download redirected:" << m_url.toString();
        return;
    }

    // if the download failed before this point ...
    if (m_state == State::Succeeded)  // pretend to succeed so we continue processing :)
    {
        qCDebug(LAUNCHER_LOG) << "Download failed but we are allowed to proceed:" << m_url.toString();
        m_sink->abort();
        m_reply.reset();
        emit succeeded();
        return;
    } else if (m_state == State::Failed) {
        qCDebug(LAUNCHER_LOG) << "Download failed in previous step:" << m_url.toString();
        m_sink->abort();
        m_reply.reset();
        emit failed("");
        return;
    } else if (m_state == State::AbortedByUser) {
        qCDebug(LAUNCHER_LOG) << "Download aborted in previous step:" << m_url.toString();
        m_sink->abort();
        m_reply.reset();
        emit aborted();
        return;
    }

    // make sure we got all the remaining data, if any
    auto data = m_reply->readAll();
    if (data.size()) {
        qCDebug(LAUNCHER_LOG) << "Writing extra" << data.size() << "bytes";
        m_state = m_sink->write(data);
    }

    // otherwise, finalize the whole graph
    m_state = m_sink->finalize(*m_reply.get());
    if (m_state != State::Succeeded) {
        qCDebug(LAUNCHER_LOG) << "Download failed to finalize:" << m_url.toString();
        m_sink->abort();
        m_reply.reset();
        emit failed("");
        return;
    }

    m_reply.reset();
    qCDebug(LAUNCHER_LOG) << "Download succeeded:" << m_url.toString();
    emit succeeded();
}

void Download::downloadReadyRead()
{
    if (m_state == State::Running) {
        auto data = m_reply->readAll();
        m_state = m_sink->write(data);
        if (m_state == State::Failed) {
            qCCritical(LAUNCHER_LOG) << "Failed to process response chunk";
        }
        // qCDebug(LAUNCHER_LOG) << "Download" << m_url.toString() << "gained" << data.size() << "bytes";
    } else {
        qCCritical(LAUNCHER_LOG) << "Cannot write download data! illegal status " << m_status;
    }
}

}  // namespace Net

auto Net::Download::abort() -> bool
{
    if (m_reply) {
        m_reply->abort();
    } else {
        m_state = State::AbortedByUser;
    }
    return true;
}
