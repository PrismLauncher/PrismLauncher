#pragma once

#include <QFuture>
#include <QFutureWatcher>
#include <QUrl>
#include "BaseInstance.h"
#include "BaseVersion.h"
#include "InstanceCopyPrefs.h"
#include "InstanceTask.h"
#include "net/NetJob.h"
#include "settings/SettingsObject.h"
#include "tasks/Task.h"

class InstanceCopyTask : public InstanceTask
{
    Q_OBJECT
public:
    explicit InstanceCopyTask(InstancePtr origInstance, InstanceCopyPrefs prefs);

protected:
    //! Entry point for tasks.
    virtual void executeTask() override;
    void copyFinished();
    void copyAborted();

private:
    // Helper functions to avoid repeating code
    static void appendToFilter(QString &filter, const QString &append);
    void resetFromMatcher(const QString &regexp);

    /* data */
    InstancePtr m_origInstance;
    QFuture<bool> m_copyFuture;
    QFutureWatcher<bool> m_copyFutureWatcher;
    std::unique_ptr<IPathMatcher> m_matcher;
    bool m_keepPlaytime;
};
