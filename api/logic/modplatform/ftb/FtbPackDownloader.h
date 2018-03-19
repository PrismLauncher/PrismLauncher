#include <QString>
#include <QUrl>
#include <QList>
#include <QObject>
#include "FtbPackFetchTask.h"
#include "tasks/Task.h"
#include "net/NetJob.h"

#include "PackHelpers.h"
#include "Env.h"

#pragma once

class MULTIMC_LOGIC_EXPORT FtbPackDownloader : public QObject
{
	Q_OBJECT

public:
	FtbPackDownloader();
	virtual ~FtbPackDownloader();

	void fetchModpacks(bool force);
	FtbModpackList getModpacks();

signals:
	void ready();
	void packFetchFailed();

private slots:
	void fetchSuccess(FtbModpackList modlist);
	void fetchFailed(QString reason);

private:
	QMap<QString, FtbModpack> fetchedPacks;
	bool fetching = false;
	bool done = false;

	FtbPackFetchTask *fetchTask = 0;
};
