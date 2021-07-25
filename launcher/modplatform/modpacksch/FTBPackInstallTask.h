#pragma once

#include "FTBPackManifest.h"

#include "InstanceTask.h"
#include "net/NetJob.h"

namespace ModpacksCH {

class PackInstallTask : public InstanceTask
{
    Q_OBJECT

public:
    explicit PackInstallTask(Modpack pack, QString version);
    virtual ~PackInstallTask(){}

    bool canAbort() const override { return true; }
    bool abort() override;

protected:
    virtual void executeTask() override;

private slots:
    void onDownloadSucceeded();
    void onDownloadFailed(QString reason);

private:
    void downloadPack();
    void install();

private:
    bool abortable = false;

    NetJobPtr jobPtr;
    QByteArray response;

    Modpack m_pack;
    QString m_version_name;
    Version m_version;

    QMap<QString, QString> filesToCopy;

};

}
