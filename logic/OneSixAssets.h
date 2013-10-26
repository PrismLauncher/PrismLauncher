#pragma once
#include "net/NetJob.h"

class Private;
class ThreadedDeleter;

class OneSixAssets : public QObject
{
	Q_OBJECT
signals:
	void failed();
	void finished();
	void indexStarted();
	void filesStarted();
	void filesProgress(int, int, int);

public slots:
	void S3BucketFinished();
	void downloadFinished();
public:
	void start();
private:
	ThreadedDeleter * deleter;
	QStringList nuke_whitelist;
	NetJobPtr index_job;
	NetJobPtr files_job;
};
