#pragma once

#include "FTBPackManifest.h"

#include "InstanceTask.h"
#include "multimc_logic_export.h"
#include "net/NetJob.h"

namespace ModpacksCH {

class MULTIMC_LOGIC_EXPORT PackInstallTask : public InstanceTask
{
    Q_OBJECT

public:
    explicit PackInstallTask(Modpack pack, QString version);
    virtual ~PackInstallTask(){}

    bool abort() override;

protected:
    virtual void executeTask() override;

private slots:
    void onDownloadSucceeded();
    void onDownloadFailed(QString reason);

private:
    void install();

private:
    NetJobPtr jobPtr;
    QByteArray response;

    Modpack m_pack;
    QString m_version_name;
    Version m_version;

};

}
