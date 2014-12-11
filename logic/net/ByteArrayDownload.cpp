/* Copyright 2013-2014 MultiMC Contributors
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

#include "ByteArrayDownload.h"
#include "MultiMC.h"
#include "logger/QsLog.h"

ByteArrayDownload::ByteArrayDownload(QUrl url) : NetAction()
{
	m_url = url;
	m_status = Job_NotStarted;
}

void ByteArrayDownload::start()
{
	QLOG_INFO() << "Downloading " << m_url.toString();
	QNetworkRequest request(m_url);
	request.setHeader(QNetworkRequest::UserAgentHeader, "MultiMC/5.0 (Uncached)");
	auto worker = MMC->qnam();
	QNetworkReply *rep = worker->get(request);

	m_reply = std::shared_ptr<QNetworkReply>(rep);
	connect(rep, SIGNAL(downloadProgress(qint64, qint64)),
			SLOT(downloadProgress(qint64, qint64)));
	connect(rep, SIGNAL(finished()), SLOT(downloadFinished()));
	connect(rep, SIGNAL(error(QNetworkReply::NetworkError)),
			SLOT(downloadError(QNetworkReply::NetworkError)));
	connect(rep, SIGNAL(readyRead()), SLOT(downloadReadyRead()));
}

void ByteArrayDownload::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	m_total_progress = bytesTotal;
	m_progress = bytesReceived;
	emit progress(m_index_within_job, bytesReceived, bytesTotal);
}

void ByteArrayDownload::downloadError(QNetworkReply::NetworkError error)
{
	// error happened during download.
	QLOG_ERROR() << "Error getting URL:" << m_url.toString().toLocal8Bit()
				 << "Network error: " << error;
	m_status = Job_Failed;
	m_errorString = m_reply->errorString();
}

void ByteArrayDownload::downloadFinished()
{
	QVariant redirect = m_reply->header(QNetworkRequest::LocationHeader);
	QString redirectURL;
	if(redirect.isValid())
	{
		redirectURL = redirect.toString();
	}
	// FIXME: This is a hack for https://bugreports.qt-project.org/browse/QTBUG-41061
	else if(m_reply->hasRawHeader("Location"))
	{
		auto data = m_reply->rawHeader("Location");
		if(data.size() > 2 && data[0] == '/' && data[1] == '/')
			redirectURL = m_reply->url().scheme() + ":" + data;
	}
	if (!redirectURL.isEmpty())
	{
		m_url = QUrl(redirect.toString());
		QLOG_INFO() << "Following redirect to " << m_url.toString();
		start();
		return;
	}

	// if the download succeeded
	if (m_status != Job_Failed)
	{
		// nothing went wrong...
		m_status = Job_Finished;
		m_data = m_reply->readAll();
		m_content_type = m_reply->header(QNetworkRequest::ContentTypeHeader).toString();
		m_reply.reset();
		emit succeeded(m_index_within_job);
		return;
	}
	// else the download failed
	else
	{
		m_reply.reset();
		emit failed(m_index_within_job);
		return;
	}
}

void ByteArrayDownload::downloadReadyRead()
{
	// ~_~
}
