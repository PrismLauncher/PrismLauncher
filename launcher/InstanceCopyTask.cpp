#include "InstanceCopyTask.h"
#include "settings/INISettingsObject.h"
#include "FileSystem.h"
#include "NullInstance.h"
#include "pathmatcher/RegexpMatcher.h"
#include <QtConcurrentRun>

InstanceCopyTask::InstanceCopyTask(InstancePtr origInstance, InstanceCopyPrefs prefs)
{
    m_origInstance = origInstance;
    m_keepPlaytime = prefs.keepPlaytime;
    QString filter;

    if(!prefs.copySaves)
    {
        appendToFilter(filter, "saves");
    }

    if(!prefs.copyGameOptions) {
        appendToFilter(filter, "options.txt");
    }

    if(!prefs.copyResourcePacks)
    {
        appendToFilter(filter, "resourcepacks");
        appendToFilter(filter, "texturepacks");
    }

    if(!prefs.copyShaderPacks)
    {
        appendToFilter(filter, "shaderpacks");
    }

    if(!prefs.copyServers)
    {
        appendToFilter(filter, "servers.dat");
        appendToFilter(filter, "servers.dat_old");
        appendToFilter(filter, "server-resource-packs");
    }

    if(!prefs.copyMods)
    {
        appendToFilter(filter, "coremods");
        appendToFilter(filter, "mods");
        appendToFilter(filter, "config");
    }

    if (!filter.isEmpty())
    {
        resetFromMatcher(filter);
    }
}

void InstanceCopyTask::appendToFilter(QString& filter, const QString &append)
{
    if (!filter.isEmpty())
        filter.append('|'); // OR regex

    filter.append("[.]?minecraft/" + append);
}

void InstanceCopyTask::resetFromMatcher(const QString& regexp)
{
    // FIXME: get this from the original instance type...
    auto matcherReal = new RegexpMatcher(regexp);
    matcherReal->caseSensitive(false);
    m_matcher.reset(matcherReal);
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
