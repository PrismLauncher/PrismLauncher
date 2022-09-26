#include "InstanceCopyTask.h"
#include "settings/INISettingsObject.h"
#include "FileSystem.h"
#include "NullInstance.h"
#include "pathmatcher/RegexpMatcher.h"
#include <QtConcurrentRun>

InstanceCopyTask::InstanceCopyTask(InstancePtr origInstance, bool copySaves, bool keepPlaytime)
{
    m_origInstance = origInstance;
    m_keepPlaytime = keepPlaytime;

    if(!copySaves)
    {
        // FIXME: get this from the original instance type...
        auto matcherReal = new RegexpMatcher("[.]?minecraft/saves");
        matcherReal->caseSensitive(false);
        m_matcher.reset(matcherReal);
    }
}

void InstanceCopyTask::executeTask()
{
    setStatus(tr("Copying instance %1").arg(m_origInstance->name()));

    FS::copy folderCopy(m_origInstance->instanceRoot(), m_stagingPath);
    folderCopy.followSymlinks(false).blacklist(m_matcher.get());

    m_copyFuture = QtConcurrent::run(QThreadPool::globalInstance(), folderCopy);
    connect(&m_copyFutureWatcher, &QFutureWatcher<bool>::finished, this, &InstanceCopyTask::copyFinished);
    connect(&m_copyFutureWatcher, &QFutureWatcher<bool>::canceled, this, &InstanceCopyTask::copyAborted);
    m_copyFutureWatcher.setFuture(m_copyFuture);
}

void InstanceCopyTask::copyFinished()
{
    auto successful = m_copyFuture.result();
    if(!successful)
    {
        emitFailed(tr("Instance folder copy failed."));
        return;
    }
    // FIXME: shouldn't this be able to report errors?
    auto instanceSettings = std::make_shared<INISettingsObject>(FS::PathCombine(m_stagingPath, "instance.cfg"));

    InstancePtr inst(new NullInstance(m_globalSettings, instanceSettings, m_stagingPath));
    inst->setName(name());
    inst->setIconKey(m_instIcon);
    if(!m_keepPlaytime) {
        inst->resetTimePlayed();
    }
    emitSucceeded();
}

void InstanceCopyTask::copyAborted()
{
    emitFailed(tr("Instance folder copy has been aborted."));
    return;
}
