#include "BaseProfiler.h"
#include "QObjectPtr.h"

#include <QProcess>

BaseProfiler::BaseProfiler(SettingsObject* settings, BaseInstance* instance, QObject* parent) : BaseExternalTool(settings, instance, parent)
{}

void BaseProfiler::beginProfiling(LaunchTask* process)
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

BaseProfiler* BaseProfilerFactory::createProfiler(BaseInstance* instance, QObject* parent)
{
    return qobject_cast<BaseProfiler*>(createTool(instance, parent));
}
