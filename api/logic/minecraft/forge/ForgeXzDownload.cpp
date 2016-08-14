/* Copyright 2013-2015 MultiMC Contributors
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

#include "Env.h"
#include "ForgeXzDownload.h"
#include <FileSystem.h>

#include <QCryptographicHash>
#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <QDebug>

ForgeXzDownload::ForgeXzDownload(QString relative_path, MetaEntryPtr entry) : NetAction()
{
	m_entry = entry;
	m_target_path = entry->getFullPath();
	m_pack200_xz_file.setFileTemplate("./dl_temp.XXXXXX");
	m_status = Job_NotStarted;
	m_url_path = relative_path;
	m_url = "http://files.minecraftforge.net/maven/" + m_url_path + ".pack.xz";
}

void ForgeXzDownload::start()
{
	if(m_status == Job_Aborted)
	{
		qWarning() << "Attempt to start an aborted Download:" << m_url.toString();
		emit aborted(m_index_within_job);
		return;
	}
	m_status = Job_InProgress;
	if (!m_entry->isStale())
	{
		m_status = Job_Finished;
		emit succeeded(m_index_within_job);
		return;
	}
	// can we actually create the real, final file?
	if (!FS::ensureFilePathExists(m_target_path))
	{
		m_status = Job_Failed;
		emit failed(m_index_within_job);
		return;
	}

	qDebug() << "Downloading " << m_url.toString();
	QNetworkRequest request(m_url);
	request.setRawHeader(QString("If-None-Match").toLatin1(), m_entry->getETag().toLatin1());
	request.setHeader(QNetworkRequest::UserAgentHeader, "MultiMC/5.0 (Cached)");

	auto worker = ENV.qnam();
	QNetworkReply *rep = worker->get(request);

	m_reply.reset(rep);
	connect(rep, SIGNAL(downloadProgress(qint64, qint64)),
			SLOT(downloadProgress(qint64, qint64)));
	connect(rep, SIGNAL(finished()), SLOT(downloadFinished()));
	connect(rep, SIGNAL(error(QNetworkReply::NetworkError)),
			SLOT(downloadError(QNetworkReply::NetworkError)));
	connect(rep, SIGNAL(readyRead()), SLOT(downloadReadyRead()));
}

void ForgeXzDownload::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	m_total_progress = bytesTotal;
	m_progress = bytesReceived;
	emit netActionProgress(m_index_within_job, bytesReceived, bytesTotal);
}

void ForgeXzDownload::downloadError(QNetworkReply::NetworkError error)
{
	if(error == QNetworkReply::OperationCanceledError)
	{
		qCritical() << "Aborted " << m_url.toString();
		m_status = Job_Aborted;
	}
	else
	{
		// error happened during download.
		qCritical() << "Failed " << m_url.toString() << " with reason " << error;
		m_status = Job_Failed;
	}
}

void ForgeXzDownload::failAndTryNextMirror()
{
	m_status = Job_Failed;
	emit failed(m_index_within_job);
}

void ForgeXzDownload::downloadFinished()
{
	// if the download succeeded
	if (m_status != Job_Failed && m_status != Job_Aborted)
	{
		// nothing went wrong...
		m_status = Job_Finished;
		if (m_pack200_xz_file.isOpen())
		{
			// we actually downloaded something! process and isntall it
			decompressAndInstall();
			return;
		}
		else
		{
			// something bad happened -- on the local machine!
			m_status = Job_Failed;
			m_pack200_xz_file.remove();
			m_reply.reset();
			emit failed(m_index_within_job);
			return;
		}
	}
	else if(m_status == Job_Aborted)
	{
		m_pack200_xz_file.remove();
		m_reply.reset();
		emit failed(m_index_within_job);
		emit aborted(m_index_within_job);
		return;
	}
	// else the download failed
	else
	{
		m_status = Job_Failed;
		m_pack200_xz_file.close();
		m_pack200_xz_file.remove();
		m_reply.reset();
		failAndTryNextMirror();
		return;
	}
}

void ForgeXzDownload::downloadReadyRead()
{

	if (!m_pack200_xz_file.isOpen())
	{
		if (!m_pack200_xz_file.open())
		{
			/*
			* Can't open the file... the job failed
			*/
			m_reply->abort();
			emit failed(m_index_within_job);
			return;
		}
	}
	m_pack200_xz_file.write(m_reply->readAll());
}

#include "xz.h"
#include "unpack200.h"
#include <stdexcept>
#include <unistd.h>

const size_t buffer_size = 8196;

// NOTE: once this gets here, it can't be aborted anymore. we don't care.
void ForgeXzDownload::decompressAndInstall()
{
	// rewind the downloaded temp file
	m_pack200_xz_file.seek(0);
	// de-xz'd file
	QTemporaryFile pack200_file("./dl_temp.XXXXXX");
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
			failAndTryNextMirror();
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
					failAndTryNextMirror();
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
				qCritical() << "Memory allocation failed\n";
				xz_dec_end(s);
				failAndTryNextMirror();
				return;

			case XZ_MEMLIMIT_ERROR:
				qCritical() << "Memory usage limit reached\n";
				xz_dec_end(s);
				failAndTryNextMirror();
				return;

			case XZ_FORMAT_ERROR:
				qCritical() << "Not a .xz file\n";
				xz_dec_end(s);
				failAndTryNextMirror();
				return;

			case XZ_OPTIONS_ERROR:
				qCritical() << "Unsupported options in the .xz headers\n";
				xz_dec_end(s);
				failAndTryNextMirror();
				return;

			case XZ_DATA_ERROR:
			case XZ_BUF_ERROR:
				qCritical() << "File is corrupt\n";
				xz_dec_end(s);
				failAndTryNextMirror();
				return;

			default:
				qCritical() << "Bug!\n";
				xz_dec_end(s);
				failAndTryNextMirror();
				return;
			}
		}
	}
	m_pack200_xz_file.remove();

	// revert pack200
	pack200_file.seek(0);
	int handle_in = pack200_file.handle();
	// FIXME: dispose of file handles, pointers and the like. Ideally wrap in objects.
	if(handle_in == -1)
	{
		qCritical() << "Error reopening " << pack200_file.fileName();
		failAndTryNextMirror();
		return;
	}
	int handle_in_dup = dup (handle_in);
	if(handle_in_dup == -1)
	{
		qCritical() << "Error reopening " << pack200_file.fileName();
		failAndTryNextMirror();
		return;
	}
	FILE *file_in = fdopen (handle_in_dup, "rb");
	if(!file_in)
	{
		qCritical() << "Error reopening " << pack200_file.fileName();
		failAndTryNextMirror();
		return;
	}
	QFile qfile_out(m_target_path);
	if(!qfile_out.open(QIODevice::WriteOnly))
	{
		qCritical() << "Error opening " << qfile_out.fileName();
		failAndTryNextMirror();
		return;
	}
	int handle_out = qfile_out.handle();
	if(handle_out == -1)
	{
		qCritical() << "Error opening " << qfile_out.fileName();
		failAndTryNextMirror();
		return;
	}
	int handle_out_dup = dup (handle_out);
	if(handle_out_dup == -1)
	{
		qCritical() << "Error reopening " << qfile_out.fileName();
		failAndTryNextMirror();
		return;
	}
	FILE *file_out = fdopen (handle_out_dup, "wb");
	if(!file_out)
	{
		qCritical() << "Error opening " << qfile_out.fileName();
		failAndTryNextMirror();
		return;
	}
	try
	{
		// NOTE: this takes ownership of both FILE pointers. That's why we duplicate them above.
		unpack_200(file_in, file_out);
	}
	catch (std::runtime_error &err)
	{
		m_status = Job_Failed;
		qCritical() << "Error unpacking " << pack200_file.fileName() << " : " << err.what();
		QFile f(m_target_path);
		if (f.exists())
			f.remove();
		failAndTryNextMirror();
		return;
	}
	pack200_file.remove();

	QFile jar_file(m_target_path);

	if (!jar_file.open(QIODevice::ReadOnly))
	{
		jar_file.remove();
		failAndTryNextMirror();
		return;
	}
	auto hash = QCryptographicHash::hash(jar_file.readAll(), QCryptographicHash::Md5);
	m_entry->setMD5Sum(hash.toHex().constData());
	jar_file.close();

	QFileInfo output_file_info(m_target_path);
	m_entry->setETag(m_reply->rawHeader("ETag").constData());
	m_entry->setLocalChangedTimestamp(output_file_info.lastModified().toUTC().toMSecsSinceEpoch());
	m_entry->setStale(false);
	ENV.metacache()->updateEntry(m_entry);

	m_reply.reset();
	emit succeeded(m_index_within_job);
}

bool ForgeXzDownload::abort()
{
	if(m_reply)
		m_reply->abort();
	m_status = Job_Aborted;
	return true;
}

bool ForgeXzDownload::canAbort()
{
	return true;
}
