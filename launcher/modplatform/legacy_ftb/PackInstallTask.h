#pragma once
#include "InstanceTask.h"
#include "net/NetJob.h"
#include <quazip/quazip.h>
#include <quazip/quazipdir.h>
#include "meta/Index.h"
#include "meta/Version.h"
#include "meta/VersionList.h"
#include "PackHelpers.h"

#include "net/NetJob.h"

#include <nonstd/optional>

namespace LegacyFTB {

class PackInstallTask : public InstanceTask
{
    Q_OBJECT

public:
    explicit PackInstallTask(shared_qobject_ptr<QNetworkAccessManager> network, Modpack pack, QString version);
    virtual ~PackInstallTask(){}

    bool canAbort() const override { return true; }
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
    shared_qobject_ptr<QNetworkAccessManager> m_network;
    bool abortable = false;
    std::unique_ptr<QuaZip> m_packZip;
    QFuture<nonstd::optional<QStringList>> m_extractFuture;
    QFutureWatcher<nonstd::optional<QStringList>> m_extractFutureWatcher;
    NetJob::Ptr netJobContainer;
    QString archivePath;

    Modpack m_pack;
    QString m_version;
};

}
