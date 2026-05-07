#include "VanillaInstanceCreationTask.h"

#include <utility>

#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "settings/INISettingsObject.h"

VanillaCreationTask::VanillaCreationTask(BaseVersion::Ptr version, QString loader, BaseVersion::Ptr loaderVersion)
    : m_version(std::move(version)), m_usingLoader(true), m_loader(std::move(loader)), m_loaderVersion(std::move(loaderVersion))
{}

void VanillaCreationTask::executeTask()
{
    setStatus(tr("Creating instance from version %1").arg(m_version->name()));

    m_instance = std::make_unique<MinecraftInstance>(
        m_globalSettings, std::make_unique<INISettingsObject>(FS::PathCombine(m_stagingPath, "instance.cfg")), m_stagingPath);
    {
        SettingsObject::Lock lock(m_instance->settings());

        auto* components = m_instance->getPackProfile();
        components->buildingFromScratch();
        components->setComponentVersion("net.minecraft", m_version->descriptor(), true);
        if (m_usingLoader) {
            components->setComponentVersion(m_loader, m_loaderVersion->descriptor());
        }

        m_instance->setName(name());
        m_instance->setIconKey(m_instIcon);

        components->saveNow();
    }

    downloadFiles(m_instance.get());
}
