#include "BaseProfiler.h"
#include "QObjectPtr.h"

#include <QProcess>

BaseProfiler::BaseProfiler(QObject* parent) : BaseExternalTool(parent)
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

BaseProfiler* BaseProfilerFactory::createProfiler(QObject* parent)
{
    return qobject_cast<BaseProfiler*>(createTool(parent));
}
