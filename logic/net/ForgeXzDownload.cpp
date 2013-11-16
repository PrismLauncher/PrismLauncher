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
#include "ForgeXzDownload.h"
#include <pathutils.h>

#include <QCryptographicHash>
#include <QFileInfo>
#include <QDateTime>
#include "logger/QsLog.h"

ForgeXzDownload::ForgeXzDownload(QUrl url, MetaEntryPtr entry) : NetAction()
{
	QString urlstr = url.toString();
	urlstr.append(".pack.xz");
	m_url = QUrl(urlstr);
	m_entry = entry;
	m_target_path = entry->getFullPath();
	m_status = Job_NotStarted;
	m_opened_for_saving = false;
}

void ForgeXzDownload::start()
{
	if (!m_entry->stale)
	{
		emit succeeded(index_within_job);
		return;
	}
	// can we actually create the real, final file?
	if (!ensureFilePathExists(m_target_path))
	{
		emit failed(index_within_job);
		return;
	}
	QLOG_INFO() << "Downloading " << m_url.toString();
	QNetworkRequest request(m_url);
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

void ForgeXzDownload::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	emit progress(index_within_job, bytesReceived, bytesTotal);
}

void ForgeXzDownload::downloadError(QNetworkReply::NetworkError error)
{
	// error happened during download.
	// TODO: log the reason why
	m_status = Job_Failed;
}

void ForgeXzDownload::downloadFinished()
{
	// if the download succeeded
	if (m_status != Job_Failed)
	{
		// nothing went wrong...
		m_status = Job_Finished;
		if (m_opened_for_saving)
		{
			// we actually downloaded something! process and isntall it
			decompressAndInstall();
			return;
		}
		else
		{
			// something bad happened
			m_status = Job_Failed;
			m_pack200_xz_file.remove();
			m_reply.reset();
			emit failed(index_within_job);
			return;
		}
	}
	// else the download failed
	else
	{
		m_pack200_xz_file.close();
		m_pack200_xz_file.remove();
		m_reply.reset();
		emit failed(index_within_job);
		return;
	}
}

void ForgeXzDownload::downloadReadyRead()
{

	if (!m_opened_for_saving)
	{
		if (!m_pack200_xz_file.open())
		{
			/*
			* Can't open the file... the job failed
			*/
			m_reply->abort();
			emit failed(index_within_job);
			return;
		}
		m_opened_for_saving = true;
	}
	m_pack200_xz_file.write(m_reply->readAll());
}

#include "xz.h"
#include "unpack200.h"
#include <stdexcept>

const size_t buffer_size = 8196;

void ForgeXzDownload::decompressAndInstall()
{
	// rewind the downloaded temp file
	m_pack200_xz_file.seek(0);
	// de-xz'd file
	QTemporaryFile pack200_file;
	pack200_file.open();

	bool xz_success = false;
	// first, de-xz
	{
		uint8_t in[buffer_size];
		uint8_t out[buffer_size];
		struct xz_buf b;
		struct xz_dec *s;
		enum xz_ret ret;
		xz_crc32_init();
		xz_crc64_init();
		s = xz_dec_init(XZ_DYNALLOC, 1 << 26);
		if (s == nullptr)
		{
			xz_dec_end(s);
			m_status = Job_Failed;
			emit failed(index_within_job);
			return;
		}
		b.in = in;
		b.in_pos = 0;
		b.in_size = 0;
		b.out = out;
		b.out_pos = 0;
		b.out_size = buffer_size;
		while (!xz_success)
		{
			if (b.in_pos == b.in_size)
			{
				b.in_size = m_pack200_xz_file.read((char *)in, sizeof(in));
				b.in_pos = 0;
			}

			ret = xz_dec_run(s, &b);

			if (b.out_pos == sizeof(out))
			{
				if (pack200_file.write((char *)out, b.out_pos) != b.out_pos)
				{
					// msg = "Write error\n";
					xz_dec_end(s);
					m_status = Job_Failed;
					emit failed(index_within_job);
					return;
				}

				b.out_pos = 0;
			}

			if (ret == XZ_OK)
				continue;

			if (ret == XZ_UNSUPPORTED_CHECK)
			{
				// unsupported check. this is OK, but we should log this
				continue;
			}

			if (pack200_file.write((char *)out, b.out_pos) != b.out_pos)
			{
				// write error
				pack200_file.close();
				xz_dec_end(s);
				return;
			}

			switch (ret)
			{
			case XZ_STREAM_END:
				xz_dec_end(s);
				xz_success = true;
				break;

			case XZ_MEM_ERROR:
				QLOG_ERROR() << "Memory allocation failed\n";
				xz_dec_end(s);
				m_status = Job_Failed;
				emit failed(index_within_job);
				return;

			case XZ_MEMLIMIT_ERROR:
				QLOG_ERROR() << "Memory usage limit reached\n";
				xz_dec_end(s);
				m_status = Job_Failed;
				emit failed(index_within_job);
				return;

			case XZ_FORMAT_ERROR:
				QLOG_ERROR() << "Not a .xz file\n";
				xz_dec_end(s);
				m_status = Job_Failed;
				emit failed(index_within_job);
				return;

			case XZ_OPTIONS_ERROR:
				QLOG_ERROR() << "Unsupported options in the .xz headers\n";
				xz_dec_end(s);
				m_status = Job_Failed;
				emit failed(index_within_job);
				return;

			case XZ_DATA_ERROR:
			case XZ_BUF_ERROR:
				QLOG_ERROR() << "File is corrupt\n";
				xz_dec_end(s);
				m_status = Job_Failed;
				emit failed(index_within_job);
				return;

			default:
				QLOG_ERROR() << "Bug!\n";
				xz_dec_end(s);
				m_status = Job_Failed;
				emit failed(index_within_job);
				return;
			}
		}
	}

	// revert pack200
	pack200_file.close();
	QString pack_name = pack200_file.fileName();
	try
	{
		unpack_200(pack_name.toStdString(), m_target_path.toStdString());
	}
	catch (std::runtime_error &err)
	{
		m_status = Job_Failed;
		QLOG_ERROR() << "Error unpacking " << pack_name.toUtf8() << " : " << err.what();
		QFile f(m_target_path);
		if (f.exists())
			f.remove();
		emit failed(index_within_job);
		return;
	}

	QFile jar_file(m_target_path);

	if (!jar_file.open(QIODevice::ReadOnly))
	{
		jar_file.remove();
		emit failed(index_within_job);
		return;
	}
	m_entry->md5sum = QCryptographicHash::hash(jar_file.readAll(), QCryptographicHash::Md5)
						  .toHex()
						  .constData();
	jar_file.close();

	QFileInfo output_file_info(m_target_path);
	m_entry->etag = m_reply->rawHeader("ETag").constData();
	m_entry->local_changed_timestamp =
		output_file_info.lastModified().toUTC().toMSecsSinceEpoch();
	m_entry->stale = false;
	MMC->metacache()->updateEntry(m_entry);

	m_reply.reset();
	emit succeeded(index_within_job);
}
