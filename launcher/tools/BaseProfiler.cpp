#include "BaseProfiler.h"
#include "QObjectPtr.h"

#include <QProcess>

BaseProfiler::BaseProfiler(SettingsObjectPtr settings, InstancePtr instance, QObject* parent) : BaseExternalTool(settings, instance, parent)
{}

void BaseProfiler::beginProfiling(shared_qobject_ptr<LaunchTask> process)
{
    beginProfilingImpl(process);
}

void BaseProfiler::abortProfiling()
{
    abortProfilingImpl();
}

void BaseProfiler::abortProfilingImpl()
{
    if (!m_profilerProcess) {
        return;
    }
    m_profilerProcess->terminate();
    m_profilerProcess->deleteLater();
    m_profilerProcess = 0;
    emit abortLaunch(tr("Profiler aborted"));
}

BaseProfiler* BaseProfilerFactory::createProfiler(InstancePtr instance, QObject* parent)
{
    return qobject_cast<BaseProfiler*>(createTool(instance, parent));
}
