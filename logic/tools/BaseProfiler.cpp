#include "BaseProfiler.h"

#include <QProcess>

BaseProfiler::BaseProfiler(InstancePtr instance, QObject *parent)
	: BaseExternalTool(instance, parent)
{
}

void BaseProfiler::beginProfiling(MinecraftProcess *process)
{
	beginProfilingImpl(process);
}

void BaseProfiler::abortProfiling()
{
	abortProfilingImpl();
}

void BaseProfiler::abortProfilingImpl()
{
	if (!m_profilerProcess)
	{
		return;
	}
	m_profilerProcess->terminate();
	m_profilerProcess->deleteLater();
	m_profilerProcess = 0;
	emit abortLaunch(tr("Profiler aborted"));
}

BaseProfiler *BaseProfilerFactory::createProfiler(InstancePtr instance, QObject *parent)
{
	return qobject_cast<BaseProfiler *>(createTool(instance, parent));
}
