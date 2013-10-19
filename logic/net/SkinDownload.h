#pragma once

#include "Download.h"
#include "HttpMetaCache.h"
#include "DownloadJob.h"
#include <QFile>
#include <QTemporaryFile>

class SkinDownload : public QObject
{
	Q_OBJECT

public:
	explicit SkinDownload(QString name);
	QString m_name;
	QUrl m_url;
	MetaEntryPtr m_entry;
	DownloadJobPtr m_job;

	void start();

protected slots:
	void downloadStarted();
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void downloadSucceeded();
	void downloadFailed();

signals:
	void started();
	void progress(qint64 current, qint64 total);
	void succeeded();
	void failed();

protected:

};

typedef std::shared_ptr<SkinDownload> SkinDownloadPtr;
