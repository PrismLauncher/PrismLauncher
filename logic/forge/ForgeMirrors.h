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

#include "logic/net/NetAction.h"
#include "logic/net/HttpMetaCache.h"
#include "logic/net/NetJob.h"
#include "logic/forge/ForgeXzDownload.h"
#include <QFile>
#include <QTemporaryFile>
typedef std::shared_ptr<class ForgeMirrors> ForgeMirrorsPtr;

class ForgeMirrors : public NetAction
{
	Q_OBJECT
public:
	QList<ForgeXzDownloadPtr> m_libs;
	NetJobPtr m_parent_job;
	QList<ForgeMirror> m_mirrors;

public:
	explicit ForgeMirrors(QList<ForgeXzDownloadPtr> &libs, NetJobPtr parent_job,
						  QString mirrorlist);
	static ForgeMirrorsPtr make(QList<ForgeXzDownloadPtr> &libs, NetJobPtr parent_job,
								QString mirrorlist)
	{
		return ForgeMirrorsPtr(new ForgeMirrors(libs, parent_job, mirrorlist));
	}
	virtual ~ForgeMirrors(){};
protected
slots:
	virtual void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	virtual void downloadError(QNetworkReply::NetworkError error);
	virtual void downloadFinished();
	virtual void downloadReadyRead();

private:
	void parseMirrorList();
	void deferToFixedList();
	void injectDownloads();

public
slots:
	virtual void start();
};
