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

#pragma once

#include "NetAction.h"
#include "HttpMetaCache.h"
#include <QCryptographicHash>
#include <QSaveFile>

typedef std::shared_ptr<class CacheDownload> CacheDownloadPtr;
class CacheDownload : public NetAction
{
	Q_OBJECT
private:
	MetaEntryPtr m_entry;
	/// if saving to file, use the one specified in this string
	QString m_target_path;
	/// this is the output file, if any
	std::shared_ptr<QSaveFile> m_output_file;
	/// the hash-as-you-download
	QCryptographicHash md5sum;

	bool wroteAnyData = false;

public:
	bool m_followRedirects = false;

	explicit CacheDownload(QUrl url, MetaEntryPtr entry);
	static CacheDownloadPtr make(QUrl url, MetaEntryPtr entry)
	{
		return CacheDownloadPtr(new CacheDownload(url, entry));
	}
	virtual ~CacheDownload(){};
	QString getTargetFilepath()
	{
		return m_target_path;
	}
protected
slots:
	virtual void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	virtual void downloadError(QNetworkReply::NetworkError error);
	virtual void downloadFinished();
	virtual void downloadReadyRead();

public
slots:
	virtual void start();
};
