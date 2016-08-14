#include "FoldersTask.h"
#include "minecraft/onesix/OneSixInstance.h"
#include <QDir>

FoldersTask::FoldersTask(OneSixInstance * inst)
{
	m_inst = inst;
}

void FoldersTask::executeTask()
{
	// Make directories
	QDir mcDir(m_inst->minecraftRoot());
	if (!mcDir.exists() && !mcDir.mkpath("."))
	{
		emitFailed(tr("Failed to create folder for minecraft binaries."));
		return;
	}
	emitSucceeded();
}
