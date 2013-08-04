#pragma once
#include "dlqueue.h"

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
	QSharedPointer<QNetworkAccessManager> net_manager {new QNetworkAccessManager()};
	JobListQueue dl;
	JobListPtr index_job;
	JobListPtr files_job;
};
