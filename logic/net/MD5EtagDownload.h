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

typedef std::shared_ptr<class MD5EtagDownload> Md5EtagDownloadPtr;
class MD5EtagDownload : public NetAction
{
	Q_OBJECT
public:
	/// the expected md5 checksum. Only set from outside
	QString m_expected_md5;
	/// the md5 checksum of a file that already exists.
	QString m_local_md5;
	/// if saving to file, use the one specified in this string
	QString m_target_path;
	/// this is the output file, if any
	QFile m_output_file;

public:
	explicit MD5EtagDownload(QUrl url, QString target_path);
	static Md5EtagDownloadPtr make(QUrl url, QString target_path)
	{
		return Md5EtagDownloadPtr(new MD5EtagDownload(url, target_path));
	}
	virtual ~MD5EtagDownload(){};
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
