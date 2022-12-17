#include "CreateGameFolders.h"
#include "minecraft/MinecraftInstance.h"
#include "launch/LaunchTask.h"
#include "FileSystem.h"

CreateGameFolders::CreateGameFolders(LaunchTask* parent): LaunchStep(parent)
{
}

void CreateGameFolders::executeTask()
{
    auto instance = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent->instance();
    std::shared_ptr<MinecraftInstance> minecraftInstance = std::dynamic_pointer_cast<MinecraftInstance>(instance);

    if(!FS::ensureFolderPathExists(minecraftInstance->gameRoot()))
    {
        emit logLine("Couldn't create the main game folder", MessageLevel::Error);
        emitFailed(tr("Couldn't create the main game folder"));
        return;
    }

    // HACK: this is a workaround for MCL-3732 - 'server-resource-packs' folder is created.
    if(!FS::ensureFolderPathExists(FS::PathCombine(minecraftInstance->gameRoot(), "server-resource-packs")))
    {
        emit logLine("Couldn't create the 'server-resource-packs' folder", MessageLevel::Error);
    }
    emitSucceeded();
}
