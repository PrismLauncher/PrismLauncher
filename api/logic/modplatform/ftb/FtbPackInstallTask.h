#pragma once
#include "InstanceTask.h"
#include "BaseInstanceProvider.h"
#include "net/NetJob.h"
#include "quazip.h"
#include "quazipdir.h"
#include "meta/Index.h"
#include "meta/Version.h"
#include "meta/VersionList.h"
#include "modplatform/ftb/PackHelpers.h"

class MULTIMC_LOGIC_EXPORT FtbPackInstallTask : public InstanceTask {

    Q_OBJECT

public:
    explicit FtbPackInstallTask(FtbModpack pack, QString version);
    virtual ~FtbPackInstallTask(){}

    bool abort() override;

protected:
    //! Entry point for tasks.
    virtual void executeTask() override;

private:
    void downloadPack();
    void unzip();
    void install();

private slots:
    void onDownloadSucceeded();
    void onDownloadFailed(QString reason);
    void onDownloadProgress(qint64 current, qint64 total);

    void onUnzipFinished();
    void onUnzipCanceled();

private: /* data */
    bool abortable = false;
    std::unique_ptr<QuaZip> m_packZip;
    QFuture<QStringList> m_extractFuture;
    QFutureWatcher<QStringList> m_extractFutureWatcher;
    NetJobPtr netJobContainer;
    QString archivePath;

    FtbModpack m_pack;
    QString m_version;
};
