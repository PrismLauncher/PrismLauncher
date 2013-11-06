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
#include <QFile>

typedef std::shared_ptr<class FileDownload> FileDownloadPtr;
class FileDownload : public NetAction
{
	Q_OBJECT
public:
	/// if true, check the md5sum against a provided md5sum
	/// also, if a file exists, perform an md5sum first and don't download only if they don't
	/// match
	bool m_check_md5;
	/// the expected md5 checksum
	QString m_expected_md5;
	/// is the saving file already open?
	bool m_opened_for_saving;
	/// if saving to file, use the one specified in this string
	QString m_target_path;
	/// this is the output file, if any
	QFile m_output_file;

public:
	explicit FileDownload(QUrl url, QString target_path);
	static FileDownloadPtr make(QUrl url, QString target_path)
	{
		return FileDownloadPtr(new FileDownload(url, target_path));
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
