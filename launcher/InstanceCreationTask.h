#pragma once

#include "tasks/Task.h"
#include "net/NetJob.h"
#include <QUrl>
#include "settings/SettingsObject.h"
#include "BaseVersion.h"
#include "InstanceTask.h"

class InstanceCreationTask : public InstanceTask
{
    Q_OBJECT
public:
    explicit InstanceCreationTask(BaseVersionPtr version);
    explicit InstanceCreationTask(BaseVersionPtr version, QString loader, BaseVersionPtr loaderVersion);

protected:
    //! Entry point for tasks.
    virtual void executeTask() override;

private: /* data */
    BaseVersionPtr m_version;
    bool m_usingLoader;
    QString m_loader;
    BaseVersionPtr m_loaderVersion;
};
