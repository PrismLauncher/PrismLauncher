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
