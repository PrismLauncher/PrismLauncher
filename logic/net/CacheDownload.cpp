/* Copyright 2013 MultiMC Contributors
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

#include "MultiMC.h"
#include "CacheDownload.h"
#include <pathutils.h>

#include <QCryptographicHash>
#include <QFileInfo>
#include <QDateTime>
#include "logger/QsLog.h"

CacheDownload::CacheDownload(QUrl url, MetaEntryPtr entry)
	: NetAction(), md5sum(QCryptographicHash::Md5)
{
	m_url = url;
	m_entry = entry;
	m_target_path = entry->getFullPath();
	m_status = Job_NotStarted;
}

void CacheDownload::start()
{
	m_status = Job_InProgress;
	if (!m_entry->stale)
	{
		m_status = Job_Finished;
		emit succeeded(m_index_within_job);
		return;
	}
	m_output_file.setFileName(m_target_path);
	// if there already is a file and md5 checking is in effect and it can be opened
	if (!ensureFilePathExists(m_target_path))
	{
		QLOG_ERROR() << "Could not create folder for " + m_target_path;
		m_status = Job_Failed;
		emit failed(m_index_within_job);
		return;
	}
	if (!m_output_file.open(QIODevice::WriteOnly))
	{
		QLOG_ERROR() << "Could not open " + m_target_path + " for writing";
		m_status = Job_Failed;
		emit failed(m_index_within_job);
		return;
	}
	QLOG_INFO() << "Downloading " << m_url.toString();
	QNetworkRequest request(m_url);
	if (m_entry->remote_changed_timestamp.size())
		request.setRawHeader(QString("If-Modified-Since").toLatin1(),
							 m_entry->remote_changed_timestamp.toLatin1());
	if (m_entry->etag.size())
		request.setRawHeader(QString("If-None-Match").toLatin1(), m_entry->etag.toLatin1());

	request.setHeader(QNetworkRequest::UserAgentHeader, "MultiMC/5.0 (Cached)");

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

void CacheDownload::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	m_total_progress = bytesTotal;
	m_progress = bytesReceived;
	emit progress(m_index_within_job, bytesReceived, bytesTotal);
}

void CacheDownload::downloadError(QNetworkReply::NetworkError error)
{
	// error happened during download.
	QLOG_ERROR() << "Failed" << m_url.toString() << "with reason" << error;
	m_status = Job_Failed;
}
void CacheDownload::downloadFinished()
{
	// if the download succeeded
	if (m_status == Job_Failed)
	{
		m_output_file.cancelWriting();
		m_reply.reset();
		m_status = Job_Failed;
		emit failed(m_index_within_job);
		return;
	}

	if (wroteAnyData)
	{
		// nothing went wrong...
		if (m_output_file.commit())
		{
			m_status = Job_Finished;
			m_entry->md5sum = md5sum.result().toHex().constData();
		}
		else
		{
			QLOG_ERROR() << "Failed to commit changes to " << m_target_path;
			m_output_file.cancelWriting();
			m_reply.reset();
			m_status = Job_Failed;
			emit failed(m_index_within_job);
			return;
		}
	}
	else
	{
		m_status = Job_Finished;
	}

	QFileInfo output_file_info(m_target_path);

	m_entry->etag = m_reply->rawHeader("ETag").constData();
	if (m_reply->hasRawHeader("Last-Modified"))
	{
		m_entry->remote_changed_timestamp = m_reply->rawHeader("Last-Modified").constData();
	}
	m_entry->local_changed_timestamp =
		output_file_info.lastModified().toUTC().toMSecsSinceEpoch();
	m_entry->stale = false;
	MMC->metacache()->updateEntry(m_entry);

	m_reply.reset();
	emit succeeded(m_index_within_job);
	return;
}

void CacheDownload::downloadReadyRead()
{
	QByteArray ba = m_reply->readAll();
	md5sum.addData(ba);
	if (m_output_file.write(ba) != ba.size())
	{
		QLOG_ERROR() << "Failed writing into " + m_target_path;
		m_status = Job_Failed;
		m_reply->abort();
		emit failed(m_index_within_job);
	}
	wroteAnyData = true;
}
