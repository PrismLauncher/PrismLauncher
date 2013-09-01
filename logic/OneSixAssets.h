#pragma once
#include "net/DownloadJob.h"

class Private;
class ThreadedDeleter;

class OneSixAssets : public QObject
{
	Q_OBJECT
signals:
	void failed();
	void finished();

public slots:
	void fetchXMLFinished();
	void downloadFinished();
public:
	void start();
private:
	ThreadedDeleter * deleter;
	QStringList nuke_whitelist;
	DownloadJobPtr index_job;
	DownloadJobPtr files_job;
};
