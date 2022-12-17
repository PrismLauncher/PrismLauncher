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
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state = State::Inactive;
}

auto Download::makeCached(QUrl url, MetaEntryPtr entry, Options options) -> Download::Ptr
{
    auto* dl = new Download();
    dl->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url = url;
    dl->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_options = options;
    auto md5Node = new ChecksumValidator(QCryptographicHash::Md5);
    auto cachedNode = new MetaCacheSink(entry, md5Node, options.testFlag(Option::MakeEternal));
    dl->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sink.reset(cachedNode);
    return dl;
}

auto Download::makeByteArray(QUrl url, QByteArray* output, Options options) -> Download::Ptr
{
    auto* dl = new Download();
    dl->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url = url;
    dl->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_options = options;
    dl->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sink.reset(new ByteArraySink(output));
    return dl;
}

auto Download::makeFile(QUrl url, QString path, Options options) -> Download::Ptr
{
    auto* dl = new Download();
    dl->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url = url;
    dl->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_options = options;
    dl->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sink.reset(new FileSink(path));
    return dl;
}

void Download::addValidator(Validator* v)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sink->addValidator(v);
}

void Download::executeTask()
{
    setStatus(tr("Downloading %1").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url.toString()));

    if (getState() == Task::State::AbortedByUser) {
        qWarning() << "Attempt to start an aborted Download:" << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url.toString();
        emitAborted();
        return;
    }

    QNetworkRequest request(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sink->init(request);
    switch (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state) {
        case State::Succeeded:
            emit succeeded();
            qDebug() << "Download cache hit " << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url.toString();
            return;
        case State::Running:
            qDebug() << "Downloading " << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url.toString();
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

    QNetworkReply* rep = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network->get(request);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply.reset(rep);
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
        qCritical() << "Aborted " << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url.toString();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state = State::AbortedByUser;
    } else {
        if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_options & Option::AcceptLocalFiles) {
            if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sink->hasLocalData()) {
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state = State::Succeeded;
                return;
            }
        }
        // error happened during download.
        qCritical() << "Failed " << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url.toString() << " with reason " << error;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state = State::Failed;
    }
}

void Download::sslErrors(const QList<QSslError>& errors)
{
    int i = 1;
    for (auto error : errors) {
        qCritical() << "Download" << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url.toString() << "SSL Error #" << i << " : " << error.errorString();
        auto cert = error.certificate();
        qCritical() << "Certificate in question:\n" << cert.toText();
        i++;
    }
}

auto Download::handleRedirect() -> bool
{
    QUrl redirect = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply->header(QNetworkRequest::LocationHeader).toUrl();
    if (!redirect.isValid()) {
        if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply->hasRawHeader("Location")) {
            // no redirect -> it's fine to continue
            return false;
        }
        // there is a Location header, but it's not correct. we need to apply some workarounds...
        QByteArray redirectBA = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply->rawHeader("Location");
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
            redirectStr = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply->url().scheme() + ":" + redirectStr;
        } else if (redirectStr.startsWith("/")) {
            /*
             * IF the URL begins with /, we need to process it as a relative URL
             */
            auto url = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply->url();
            url.setPath(redirectStr, QUrl::TolerantMode);
            redirectStr = url.toString();
        }

        /*
         * Next, make sure the URL is parsed in tolerant mode. Qt doesn't parse the location header in tolerant mode, which causes issues.
         * FIXME: report Qt bug for this
         */
        redirect = QUrl(redirectStr, QUrl::TolerantMode);
        if (!redirect.isValid()) {
            qWarning() << "Failed to parse redirect URL:" << redirectStr;
            downloadError(QNetworkReply::ProtocolFailure);
            return false;
        }
        qDebug() << "Fixed location header:" << redirect;
    } else {
        qDebug() << "Location header:" << redirect;
    }

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url = QUrl(redirect.toString());
    qDebug() << "Following redirect to " << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url.toString();
    startAction(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network);

    return true;
}

void Download::downloadFinished()
{
    // handle HTTP redirection first
    if (handleRedirect()) {
        qDebug() << "Download redirected:" << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url.toString();
        return;
    }

    // if the download failed before this point ...
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state == State::Succeeded)  // pretend to succeed so we continue processing :)
    {
        qDebug() << "Download failed but we are allowed to proceed:" << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url.toString();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sink->abort();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply.reset();
        emit succeeded();
        return;
    } else if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state == State::Failed) {
        qDebug() << "Download failed in previous step:" << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url.toString();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sink->abort();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply.reset();
        emit failed("");
        return;
    } else if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state == State::AbortedByUser) {
        qDebug() << "Download aborted in previous step:" << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url.toString();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sink->abort();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply.reset();
        emit aborted();
        return;
    }

    // make sure we got all the remaining data, if any
    auto data = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply->readAll();
    if (data.size()) {
        qDebug() << "Writing extra" << data.size() << "bytes";
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sink->write(data);
    }

    // otherwise, finalize the whole graph
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sink->finalize(*hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply.get());
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state != State::Succeeded) {
        qDebug() << "Download failed to finalize:" << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url.toString();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sink->abort();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply.reset();
        emit failed("");
        return;
    }

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply.reset();
    qDebug() << "Download succeeded:" << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url.toString();
    emit succeeded();
}

void Download::downloadReadyRead()
{
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state == State::Running) {
        auto data = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply->readAll();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sink->write(data);
        if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state == State::Failed) {
            qCritical() << "Failed to process response chunk";
        }
        // qDebug() << "Download" << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_url.toString() << "gained" << data.size() << "bytes";
    } else {
        qCritical() << "Cannot write download data! illegal status " << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_status;
    }
}

}  // namespace Net

auto Net::Download::abort() -> bool
{
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply) {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply->abort();
    } else {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_state = State::AbortedByUser;
    }
    return true;
}
