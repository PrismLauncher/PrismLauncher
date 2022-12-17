#include "VanillaInstanceCreationTask.h"

#include <utility>

#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "settings/INISettingsObject.h"

VanillaCreationTask::VanillaCreationTask(BaseVersion::Ptr version, QString loader, BaseVersion::Ptr loader_version)
    : InstanceCreationTask(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version(std::move(version)), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_using_loader(true), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loader(std::move(loader)), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loader_version(std::move(loader_version))
{}

bool VanillaCreationTask::createInstance()
{
    setStatus(tr("Creating instance from version %1").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version->name()));

    auto instance_settings = std::make_shared<INISettingsObject>(FS::PathCombine(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stagingPath, "instance.cfg"));
    instance_settings->suspendSave();
    {
        MinecraftInstance inst(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_globalSettings, instance_settings, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stagingPath);
        auto components = inst.getPackProfile();
        components->buildingFromScratch();
        components->setComponentVersion("net.minecraft", hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version->descriptor(), true);
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_using_loader)
            components->setComponentVersion(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loader, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loader_version->descriptor());

        inst.setName(name());
        inst.setIconKey(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instIcon);
    }
    instance_settings->resumeSave();

    return true;
}
