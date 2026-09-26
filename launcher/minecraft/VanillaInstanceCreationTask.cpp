#include "VanillaInstanceCreationTask.h"

#include <utility>

#include "FileSystem.h"
#include "config/InstanceConfig.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

VanillaCreationTask::VanillaCreationTask(BaseVersion::Ptr version, QString loader, BaseVersion::Ptr loaderVersion)
    : m_version(std::move(version)), m_usingLoader(true), m_loader(std::move(loader)), m_loaderVersion(std::move(loaderVersion))
{}

void VanillaCreationTask::executeTask()
{
    setStatus(tr("Creating instance from version %1").arg(m_version->name()));

    auto confTmp = std::make_unique<InstanceConfigHolder>(FS::PathCombine(m_stagingPath, "instance.cfg"), InstanceConfig::loadDefaults());
    m_instance = std::make_unique<MinecraftInstance>(std::move(confTmp), m_stagingPath);
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

    if (const auto saveResult = m_instance->config().save(); !saveResult) {
        emitFailed(tr("Failed to save instance config: %1").arg(saveResult.error()));
        return;
    }

    downloadFiles(m_instance.get());
}
