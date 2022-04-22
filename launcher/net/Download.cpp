/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Download.h"

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>

#include "ByteArraySink.h"
#include "ChecksumValidator.h"
#include "FileSystem.h"
#include "MetaCacheSink.h"

#include "BuildConfig.h"

namespace Net {

Download::Download() : NetAction()
{
    m_state = State::Inactive;
}

Download::Ptr Download::makeCached(QUrl url, MetaEntryPtr entry, Options options)
{
    Download* dl = new Download();
    dl->m_url = url;
    dl->m_options = options;
    auto md5Node = new ChecksumValidator(QCryptographicHash::Md5);
    auto cachedNode = new MetaCacheSink(entry, md5Node);
    dl->m_sink.reset(cachedNode);
    dl->m_target_path = entry->getFullPath();
    return dl;
}

Download::Ptr Download::makeByteArray(QUrl url, QByteArray* output, Options options)
{
    Download* dl = new Download();
    dl->m_url = url;
    dl->m_options = options;
    dl->m_sink.reset(new ByteArraySink(output));
    return dl;
}

Download::Ptr Download::makeFile(QUrl url, QString path, Options options)
{
    Download* dl = new Download();
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
    if (getState() == Task::State::AbortedByUser) {
        qWarning() << "Attempt to start an aborted Download:" << m_url.toString();
        emit aborted(m_index_within_job);
        return;
    }

    QNetworkRequest request(m_url);
    m_state = m_sink->init(request);
    switch (m_state) {
        case State::Succeeded:
            emit succeeded(m_index_within_job);
            qDebug() << "Download cache hit " << m_url.toString();
            return;
        case State::Running:
            qDebug() << "Downloading " << m_url.toString();
            break;
        case State::Inactive:
        case State::Failed:
            emit failed(m_index_within_job);
            return;
        case State::AbortedByUser:
            return;
    }

    request.setHeader(QNetworkRequest::UserAgentHeader, BuildConfig.USER_AGENT);
    if (request.url().host().contains("api.curseforge.com")) {
        request.setRawHeader("x-api-key", BuildConfig.CURSEFORGE_API_KEY.toUtf8());
    };

    QNetworkReply* rep = m_network->get(request);

    m_reply.reset(rep);
    connect(rep, SIGNAL(downloadProgress(qint64, qint64)), SLOT(downloadProgress(qint64, qint64)));
    connect(rep, SIGNAL(finished()), SLOT(downloadFinished()));
    connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(downloadError(QNetworkReply::NetworkError)));
    connect(rep, &QNetworkReply::sslErrors, this, &Download::sslErrors);
    connect(rep, &QNetworkReply::readyRead, this, &Download::downloadReadyRead);
}

void Download::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    setProgress(bytesReceived, bytesTotal);
    emit netActionProgress(m_index_within_job, bytesReceived, bytesTotal);
}

void Download::downloadError(QNetworkReply::NetworkError error)
{
    if (error == QNetworkReply::OperationCanceledError) {
        qCritical() << "Aborted " << m_url.toString();
        m_state = State::AbortedByUser;
    } else {
        if (m_options & Option::AcceptLocalFiles) {
            if (m_sink->hasLocalData()) {
                m_state = State::Succeeded;
                return;
            }
        }
        // error happened during download.
        qCritical() << "Failed " << m_url.toString() << " with reason " << error;
        m_state = State::Failed;
    }
}

void Download::sslErrors(const QList<QSslError>& errors)
{
    int i = 1;
    for (auto error : errors) {
        qCritical() << "Download" << m_url.toString() << "SSL Error #" << i << " : " << error.errorString();
        auto cert = error.certificate();
        qCritical() << "Certificate in question:\n" << cert.toText();
        i++;
    }
}

bool Download::handleRedirect()
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
            qWarning() << "Failed to parse redirect URL:" << redirectStr;
            downloadError(QNetworkReply::ProtocolFailure);
            return false;
        }
        qDebug() << "Fixed location header:" << redirect;
    } else {
        qDebug() << "Location header:" << redirect;
    }

    m_url = QUrl(redirect.toString());
    qDebug() << "Following redirect to " << m_url.toString();
    startAction(m_network);

    return true;
}

void Download::downloadFinished()
{
    // handle HTTP redirection first
    if (handleRedirect()) {
        qDebug() << "Download redirected:" << m_url.toString();
        return;
    }

    // if the download failed before this point ...
    if (m_state == State::Succeeded)  // pretend to succeed so we continue processing :)
    {
        qDebug() << "Download failed but we are allowed to proceed:" << m_url.toString();
        m_sink->abort();
        m_reply.reset();
        emit succeeded(m_index_within_job);
        return;
    } else if (m_state == State::Failed) {
        qDebug() << "Download failed in previous step:" << m_url.toString();
        m_sink->abort();
        m_reply.reset();
        emit failed(m_index_within_job);
        return;
    } else if (m_state == State::AbortedByUser) {
        qDebug() << "Download aborted in previous step:" << m_url.toString();
        m_sink->abort();
        m_reply.reset();
        emit aborted(m_index_within_job);
        return;
    }

    // make sure we got all the remaining data, if any
    auto data = m_reply->readAll();
    if (data.size()) {
        qDebug() << "Writing extra" << data.size() << "bytes to" << m_target_path;
        m_state = m_sink->write(data);
    }

    // otherwise, finalize the whole graph
    m_state = m_sink->finalize(*m_reply.get());
    if (m_state != State::Succeeded) {
        qDebug() << "Download failed to finalize:" << m_url.toString();
        m_sink->abort();
        m_reply.reset();
        emit failed(m_index_within_job);
        return;
    }
    m_reply.reset();
    qDebug() << "Download succeeded:" << m_url.toString();
    emit succeeded(m_index_within_job);
}

void Download::downloadReadyRead()
{
    if (m_state == State::Running) {
        auto data = m_reply->readAll();
        m_state = m_sink->write(data);
        if (m_state == State::Failed) {
            qCritical() << "Failed to process response chunk for " << m_target_path;
        }
        // qDebug() << "Download" << m_url.toString() << "gained" << data.size() << "bytes";
    } else {
        qCritical() << "Cannot write to " << m_target_path << ", illegal status" << m_status;
    }
}

}  // namespace Net

bool Net::Download::abort()
{
    if (m_reply) {
        m_reply->abort();
    } else {
        m_state = State::AbortedByUser;
    }
    return true;
}
