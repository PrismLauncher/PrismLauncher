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
#include "HttpMetaCache.h"
#include "Validator.h"
#include "Sink.h"

#include "multimc_logic_export.h"
namespace Net {
class MULTIMC_LOGIC_EXPORT Download : public NetAction
{
	Q_OBJECT

public: /* types */
	typedef std::shared_ptr<class Download> Ptr;

protected: /* con/des */
	explicit Download();
public:
	virtual ~Download(){};
	static Download::Ptr makeCached(QUrl url, MetaEntryPtr entry);
	static Download::Ptr makeByteArray(QUrl url, QByteArray *output);
	static Download::Ptr makeFile(QUrl url, QString path);

public: /* methods */
	// FIXME: remove this
	QString getTargetFilepath()
	{
		return m_target_path;
	}
	// FIXME: remove this
	void addValidator(Validator * v);

private: /* methods */
	bool handleRedirect();

protected slots:
	virtual void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	virtual void downloadError(QNetworkReply::NetworkError error);
	virtual void downloadFinished();
	virtual void downloadReadyRead();

public slots:
	virtual void start();

private: /* data */
	// FIXME: remove this, it has no business being here.
	QString m_target_path;
	std::unique_ptr<Sink> m_sink;
};
}