#pragma once

#include <QFuture>
#include <QFutureWatcher>
#include <QUrl>
#include "Filter.h"
#include "InstanceCopyPrefs.h"
#include "InstanceTask.h"
#include "minecraft/MinecraftInstance.h"

class InstanceCopyTask : public InstanceTask {
    Q_OBJECT
   public:
    explicit InstanceCopyTask(MinecraftInstance* origInstance, const InstanceCopyPrefs& prefs);

   protected:
    //! Entry point for tasks.
    void executeTask() override;
    bool abort() override;
    void copyFinished();
    void copyAborted();

   private:
    /* data */
    MinecraftInstance* m_origInstance;
    QFuture<bool> m_copyFuture;
    QFutureWatcher<bool> m_copyFutureWatcher;
    Filter m_matcher;
    bool m_keepPlaytime;
    bool m_useLinks = false;
    bool m_useHardLinks = false;
    bool m_copySaves = false;
    bool m_linkRecursively = false;
    bool m_useClone = false;
};
