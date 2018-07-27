#include "CreateServerResourcePacksFolder.h"
#include "minecraft/MinecraftInstance.h"
#include "launch/LaunchTask.h"
#include "FileSystem.h"

CreateServerResourcePacksFolder::CreateServerResourcePacksFolder(LaunchTask* parent): LaunchStep(parent)
{
}

void CreateServerResourcePacksFolder::executeTask()
{
    auto instance = m_parent->instance();
    std::shared_ptr<MinecraftInstance> minecraftInstance = std::dynamic_pointer_cast<MinecraftInstance>(instance);
    if(!FS::ensureFolderPathExists(FS::PathCombine(minecraftInstance->gameRoot(), "server-resource-packs")))
    {
        emit logLine(tr("Couldn't create the 'server-resource-packs' folder"), MessageLevel::Error);
    }
    emitSucceeded();
}
