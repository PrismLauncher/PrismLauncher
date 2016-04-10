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

#pragma once
#include "NetAction.h"

#include "multimc_logic_export.h"

typedef std::shared_ptr<class ByteArrayDownload> ByteArrayDownloadPtr;
class MULTIMC_LOGIC_EXPORT ByteArrayDownload : public NetAction
{
	Q_OBJECT
public:
	ByteArrayDownload(QUrl url);
	static ByteArrayDownloadPtr make(QUrl url)
	{
		return ByteArrayDownloadPtr(new ByteArrayDownload(url));
	}
    virtual ~ByteArrayDownload() {};
public:
	/// if not saving to file, downloaded data is placed here
	QByteArray m_data;

	QString m_errorString;

public
slots:
	virtual void start();

protected
slots:
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void downloadError(QNetworkReply::NetworkError error);
	void downloadFinished();
	void downloadReadyRead();
};
