#include "FoldersTask.h"
#include "minecraft/MinecraftInstance.h"
#include <QDir>

FoldersTask::FoldersTask(MinecraftInstance * inst)
    :Task()
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
