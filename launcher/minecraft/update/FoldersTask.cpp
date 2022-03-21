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
    QDir mcDir(m_inst->gameRoot());
    if (!mcDir.exists() && !mcDir.mkpath("."))
    {
        emitFailed(tr("Failed to create folder for Minecraft binaries."));
        return;
    }
    emitSucceeded();
}
