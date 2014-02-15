#include "BaseProfiler.h"

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

BaseProfilerFactory::~BaseProfilerFactory()
{
}
