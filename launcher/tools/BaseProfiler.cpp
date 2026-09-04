#include "BaseProfiler.h"

#include <QProcess>

BaseProfiler::BaseProfiler(SettingsObject* settings, MinecraftInstance* instance, QObject* parent)
    : BaseExternalTool(settings, instance, parent)
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
    m_profilerProcess = nullptr;
    emit abortLaunch(tr("Profiler aborted"));
}

BaseProfiler* BaseProfilerFactory::createProfiler(MinecraftInstance* instance, QObject* parent)
{
    return qobject_cast<BaseProfiler*>(createTool(instance, parent));
}
