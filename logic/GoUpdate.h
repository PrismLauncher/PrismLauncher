#pragma once
#include "net/NetJob.h"

class GoUpdate : public QObject
{
	Q_OBJECT

public:
	struct version_channel
	{
		QString id;
		QString name;
		int latestVersion;
	};

	struct version_summary
	{
		int id;
		QString name;
	};

signals:
	void updateAvailable();

private slots:
	void updateCheckFinished();
	void updateCheckFailed();

public:
	GoUpdate();
	void checkForUpdate();
private:
	NetJobPtr index_job;
	NetJobPtr fromto_job;

	QString repoUrlBase;
	QString builderName;
	int currentBuildIndex;
	int newBuildIndex = -1;

	QList<version_summary> versions;
	QList<version_channel> channels;
};
