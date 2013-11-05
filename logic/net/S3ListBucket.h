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

struct S3Object
{
	QString Key;
	QString ETag;
	qlonglong size;
};

typedef std::shared_ptr<class S3ListBucket> S3ListBucketPtr;
class S3ListBucket : public NetAction
{
	Q_OBJECT
public:
	S3ListBucket(QUrl url);
	static S3ListBucketPtr make(QUrl url)
	{
		return S3ListBucketPtr(new S3ListBucket(url));
	}

public:
	QList<S3Object> objects;

public
slots:
	virtual void start() override;

protected
slots:
	virtual void downloadProgress(qint64 bytesReceived, qint64 bytesTotal) override;
	virtual void downloadError(QNetworkReply::NetworkError error) override;
	virtual void downloadFinished() override;
	virtual void downloadReadyRead() override;

private:
	void processValidReply();

private:
	qint64 bytesSoFar = 0;
	QString current_marker;
};
