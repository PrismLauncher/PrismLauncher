#include "BaseProfiler.h"

#include <QProcess>

BaseProfiler::BaseProfiler(OneSixInstance *instance, QObject *parent)
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

qint64 BaseProfiler::pid(QProcess *process)
{
#ifdef Q_OS_UNIX
	return process->pid();
#else
	return (qint64)process->pid();
#endif
}

BaseProfilerFactory::~BaseProfilerFactory()
{
}
