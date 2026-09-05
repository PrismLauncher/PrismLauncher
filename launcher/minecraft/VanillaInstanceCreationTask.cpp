#include "VanillaInstanceCreationTask.h"

#include <utility>

#include "config/InstanceConfig.h"
#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

VanillaCreationTask::VanillaCreationTask(BaseVersion::Ptr version, QString loader, BaseVersion::Ptr loaderVersion)
    : m_version(std::move(version)), m_usingLoader(true), m_loader(std::move(loader)), m_loaderVersion(std::move(loaderVersion))
{}

void VanillaCreationTask::executeTask()
{
    setStatus(tr("Creating instance from version %1").arg(m_version->name()));

    auto conf = std::make_unique<InstanceConfigHolder>(FS::PathCombine(m_stagingPath, "instance.cfg"));
    m_instance = std::make_unique<MinecraftInstance>(std::move(conf), m_stagingPath);
    {
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

    m_instance->config().save();

    downloadFiles(m_instance.get());
}
