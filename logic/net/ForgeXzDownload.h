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
#include <QFile>
#include <QTemporaryFile>
typedef std::shared_ptr<class ForgeXzDownload> ForgeXzDownloadPtr;

class ForgeXzDownload : public NetAction
{
	Q_OBJECT
public:
	MetaEntryPtr m_entry;
	/// is the saving file already open?
	bool m_opened_for_saving;
	/// if saving to file, use the one specified in this string
	QString m_target_path;
	/// this is the output file, if any
	QTemporaryFile m_pack200_xz_file;

public:
	explicit ForgeXzDownload(QUrl url, MetaEntryPtr entry);
	static ForgeXzDownloadPtr make(QUrl url, MetaEntryPtr entry)
	{
		return ForgeXzDownloadPtr(new ForgeXzDownload(url, entry));
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

private:
	void decompressAndInstall();
};
