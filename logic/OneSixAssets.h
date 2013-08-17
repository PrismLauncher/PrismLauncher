#pragma once
#include "net/DownloadJob.h"

class Private;

class OneSixAssets : public QObject
{
	Q_OBJECT
signals:
	void failed();
	void finished();

public slots:
	void fetchFinished();
	void fetchStarted();
public:
	void start();
private:
	JobListQueue dl;
	JobListPtr index_job;
	JobListPtr files_job;
};
