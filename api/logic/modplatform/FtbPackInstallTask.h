#pragma once
#include "tasks/Task.h"
#include "modplatform/FtbPackDownloader.h"
#include "BaseInstanceProvider.h"
#include "net/NetJob.h"
#include "quazip.h"
#include "quazipdir.h"

class MULTIMC_LOGIC_EXPORT FtbPackInstallTask : public Task {

	Q_OBJECT

public:
	explicit FtbPackInstallTask(FtbPackDownloader *downloader, SettingsObjectPtr settings, const QString & stagingPath, const QString &instName,
				    const QString &instIcon, const QString &instGroup);

protected:
	//! Entry point for tasks.
	virtual void executeTask() override;

private: /* data */
	SettingsObjectPtr m_globalSettings;
	QString m_stagingPath;
	QString m_instName;
	QString m_instIcon;
	QString m_instGroup;
	NetJobPtr m_netJobPtr;
	FtbPackDownloader *m_downloader = nullptr;

	std::unique_ptr<QuaZip> m_packZip;
	QFuture<QStringList> m_extractFuture;
	QFutureWatcher<QStringList> m_extractFutureWatcher;

	void downloadPack();
	void unzip(QString archivePath);
	void install();

private slots:
	void onDownloadSucceeded(QString archivePath);
	void onDownloadFailed(QString reason);
	void onDownloadProgress(qint64 current, qint64 total);

	void onUnzipFinished();
	void onUnzipCanceled();
};
