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

#include "net/NetAction.h"
#include "net/HttpMetaCache.h"
#include <QFile>
#include <QTemporaryFile>

typedef std::shared_ptr<class ForgeXzDownload> ForgeXzDownloadPtr;

class ForgeXzDownload : public NetAction
{
	Q_OBJECT
public:
	MetaEntryPtr m_entry;
	/// if saving to file, use the one specified in this string
	QString m_target_path;
	/// this is the output file, if any
	QTemporaryFile m_pack200_xz_file;
	/// path relative to the mirror base
	QString m_url_path;

public:
	explicit ForgeXzDownload(QString relative_path, MetaEntryPtr entry);
	static ForgeXzDownloadPtr make(QString relative_path, MetaEntryPtr entry)
	{
		return ForgeXzDownloadPtr(new ForgeXzDownload(relative_path, entry));
	}
	virtual ~ForgeXzDownload(){};
	bool canAbort() override;

protected
slots:
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal) override;
	void downloadError(QNetworkReply::NetworkError error) override;
	void downloadFinished() override;
	void downloadReadyRead() override;

public
slots:
	void start() override;
	bool abort() override;

private:
	void decompressAndInstall();
	void failAndTryNextMirror();
};
