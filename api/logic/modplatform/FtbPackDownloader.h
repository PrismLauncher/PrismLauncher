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

class FtbPackDownloader;
class MULTIMC_LOGIC_EXPORT FtbPackDownloader : public QObject {

	Q_OBJECT

private:
	QMap<QString, FtbModpack> fetchedPacks;
	bool fetching;
	bool done;

	FtbModpack selected;
	QString selectedVersion;
	QString downloadPath;

	FtbPackFetchTask *fetchTask = 0;
	NetJobPtr netJobContainer;

	void _downloadSucceeded();
	void _downloadFailed(QString reason);
	void _downloadProgress(qint64 current, qint64 total);

private slots:
	void fetchSuccess(FtbModpackList modlist);
	void fetchFailed(QString reason);

public:
	FtbPackDownloader();
	~FtbPackDownloader();

	bool isValidPackSelected();
	void selectPack(FtbModpack modpack, QString version);

	FtbModpack getSelectedPack();

	void fetchModpacks(bool force);
	void downloadSelected(MetaEntryPtr cache);

	QString getSuggestedInstanceName();

	FtbModpackList getModpacks();

signals:
	void ready();
	void packFetchFailed();

	void downloadSucceded(QString archivePath);
	void downloadFailed(QString reason);
	void downloadProgress(qint64 current, qint64 total);

};
