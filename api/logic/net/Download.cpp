/* Copyright 2013-2017 MultiMC Contributors
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

#include <QFileInfo>
#include <QDateTime>
#include <QDebug>
#include "Env.h"
#include <FileSystem.h>
#include "ChecksumValidator.h"
#include "MetaCacheSink.h"
#include "ByteArraySink.h"

namespace Net {

Download::Download():NetAction()
{
	m_status = Status::NotStarted;
}

Download::Ptr Download::makeCached(QUrl url, MetaEntryPtr entry, Options options)
{
	Download * dl = new Download();
	dl->m_url = url;
	dl->m_options = options;
	auto md5Node = new ChecksumValidator(QCryptographicHash::Md5);
	auto cachedNode = new MetaCacheSink(entry, md5Node);
	dl->m_sink.reset(cachedNode);
	dl->m_target_path = entry->getFullPath();
	return std::shared_ptr<Download>(dl);
}

Download::Ptr Download::makeByteArray(QUrl url, QByteArray *output, Options options)
{
	Download * dl = new Download();
	dl->m_url = url;
	dl->m_options = options;
	dl->m_sink.reset(new ByteArraySink(output));
	return std::shared_ptr<Download>(dl);
}

Download::Ptr Download::makeFile(QUrl url, QString path, Options options)
{
	Download * dl = new Download();
	dl->m_url = url;
	dl->m_options = options;
	dl->m_sink.reset(new FileSink(path));
	return std::shared_ptr<Download>(dl);
}

void Download::addValidator(Validator * v)
{
	m_sink->addValidator(v);
}

void Download::executeTask()
{
	if(m_status == Status::Aborted)
	{
		qWarning() << "Attempt to start an aborted Download:" << m_url.toString();
		emit aborted();
		return;
	}
	QNetworkRequest request(m_url);
	m_status = m_sink->init(request);
	switch(m_status)
	{
		case Status::Finished:
			emit succeeded();
			qDebug() << "Download cache hit " << m_url.toString();
			return;
		case Status::InProgress:
			qDebug() << "Downloading " << m_url.toString();
			break;
		case Status::NotStarted:
		case Status::Failed:
			emit failed();
			return;
		case Status::Aborted:
			return;
	}

	request.setHeader(QNetworkRequest::UserAgentHeader, "MultiMC/5.0");

	QNetworkReply *rep =  ENV.qnam().get(request);

	m_reply.reset(rep);
	connect(rep, SIGNAL(downloadProgress(qint64, qint64)), SLOT(downloadProgress(qint64, qint64)));
	connect(rep, SIGNAL(finished()), SLOT(downloadFinished()));
	connect(rep, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(downloadError(QNetworkReply::NetworkError)));
	connect(rep, SIGNAL(readyRead()), SLOT(downloadReadyRead()));
}

void Download::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	m_progressTotal = bytesTotal;
	m_progress = bytesReceived;
	emit progress(bytesReceived, bytesTotal);
}

void Download::downloadError(QNetworkReply::NetworkError error)
{
	if(error == QNetworkReply::OperationCanceledError)
	{
		qCritical() << "Aborted " << m_url.toString();
		m_status = Status::Aborted;
	}
	else
	{
		if(m_options & Option::AcceptLocalFiles)
		{
			if(m_sink->hasLocalData())
			{
				m_status = Status::Failed_Proceed;
				return;
			}
		}
		// error happened during download.
		qCritical() << "Failed " << m_url.toString() << " with reason " << error;
		m_status = Status::Failed;
	}
}

bool Download::handleRedirect()
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
		{
			redirectURL = m_reply->url().scheme() + ":" + data;
		}
	}
	if (!redirectURL.isEmpty())
	{
		m_url = QUrl(redirect.toString());
		qDebug() << "Following redirect to " << m_url.toString();
		start();
		return true;
	}
	return false;
}


void Download::downloadFinished()
{
	// handle HTTP redirection first
	if(handleRedirect())
	{
		qDebug() << "Download redirected:" << m_url.toString();
		return;
	}

	// if the download failed before this point ...
	if (m_status == Status::Failed_Proceed)
	{
		qDebug() << "Download failed but we are allowed to proceed:" << m_url.toString();
		m_sink->abort();
		m_reply.reset();
		emit succeeded();
		return;
	}
	else if (m_status == Status::Failed)
	{
		qDebug() << "Download failed in previous step:" << m_url.toString();
		m_sink->abort();
		m_reply.reset();
		emit failed();
		return;
	}
	else if(m_status == Status::Aborted)
	{
		qDebug() << "Download aborted in previous step:" << m_url.toString();
		m_sink->abort();
		m_reply.reset();
		emit aborted();
		return;
	}

	// make sure we got all the remaining data, if any
	auto data = m_reply->readAll();
	if(data.size())
	{
		qDebug() << "Writing extra" << data.size() << "bytes to" << m_target_path;
		m_status = m_sink->write(data);
	}

	// otherwise, finalize the whole graph
	m_status = m_sink->finalize(*m_reply.get());
	if (m_status != Status::Finished)
	{
		qDebug() << "Download failed to finalize:" << m_url.toString();
		m_sink->abort();
		m_reply.reset();
		emit failed();
		return;
	}
	m_reply.reset();
	qDebug() << "Download succeeded:" << m_url.toString();
	emit succeeded();
}

void Download::downloadReadyRead()
{
	if(m_status == Status::InProgress)
	{
		auto data = m_reply->readAll();
		m_status = m_sink->write(data);
		if(m_status == Status::Failed)
		{
			qCritical() << "Failed to process response chunk for " << m_target_path;
		}
		// qDebug() << "Download" << m_url.toString() << "gained" << data.size() << "bytes";
	}
	else
	{
		qCritical() << "Cannot write to " << m_target_path << ", illegal status" << int(m_status);
	}
}

}

bool Net::Download::abort()
{
	if(m_reply)
	{
		m_reply->abort();
	}
	else
	{
		m_status = Status::Aborted;
	}
	return true;
}

bool Net::Download::canAbort() const
{
	return true;
}
