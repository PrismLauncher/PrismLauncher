#include "VanillaInstanceCreationTask.h"

#include <utility>

#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "settings/INISettingsObject.h"

VanillaCreationTask::VanillaCreationTask(BaseVersionPtr version, QString loader, BaseVersionPtr loader_version)
    : InstanceCreationTask(), m_version(std::move(version)), m_using_loader(true), m_loader(std::move(loader)), m_loader_version(std::move(loader_version))
{}

bool VanillaCreationTask::createInstance()
{
    setStatus(tr("Creating instance from version %1").arg(m_version->name()));

    auto instance_settings = std::make_shared<INISettingsObject>(FS::PathCombine(m_stagingPath, "instance.cfg"));
    instance_settings->suspendSave();
    {
        MinecraftInstance inst(m_globalSettings, instance_settings, m_stagingPath);
        auto components = inst.getPackProfile();
        components->buildingFromScratch();
        components->setComponentVersion("net.minecraft", m_version->descriptor(), true);
        if(m_using_loader)
            components->setComponentVersion(m_loader, m_loader_version->descriptor());

        inst.setName(name());
        inst.setIconKey(m_instIcon);
    }
    instance_settings->resumeSave();

    return true;
}
