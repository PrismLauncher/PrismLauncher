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

#include <optional>

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
    void onDownloadAborted();

    void onUnzipFinished();
    void onUnzipCanceled();

private: /* data */
    shared_qobject_ptr<QNetworkAccessManager> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network;
    bool abortable = false;
    std::unique_ptr<QuaZip> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_packZip;
    QFuture<std::optional<QStringList>> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractFuture;
    QFutureWatcher<std::optional<QStringList>> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractFutureWatcher;
    NetJob::Ptr netJobContainer;
    QString archivePath;

    Modpack hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pack;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version;
};

}
