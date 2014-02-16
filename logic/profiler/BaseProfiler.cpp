#include "BaseProfiler.h"

#include <QProcess>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

BaseProfiler::BaseProfiler(BaseInstance *instance, QObject *parent)
	: QObject(parent), m_instance(instance)
{
}

BaseProfiler::~BaseProfiler()
{
}

void BaseProfiler::beginProfiling(MinecraftProcess *process)
{
	beginProfilingImpl(process);
}

void BaseProfiler::abortProfiling()
{
	abortProfiling();
}

void BaseProfiler::abortProfilingImpl()
{
	if (!m_profilerProcess)
	{
		return;
	}
	m_profilerProcess->terminate();
	m_profilerProcess->deleteLater();
}

qint64 BaseProfiler::pid(QProcess *process)
{
#ifdef Q_OS_WIN
	struct _PROCESS_INFORMATION *procinfo = process->pid();
	return procinfo->dwProcessId;
#else
	return process->pid();
#endif
}

BaseProfilerFactory::~BaseProfilerFactory()
{
}
