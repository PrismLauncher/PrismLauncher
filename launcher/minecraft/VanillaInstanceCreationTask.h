#pragma once

#include <memory>
#include "BaseVersion.h"
#include "InstanceTask.h"
#include "minecraft/MinecraftInstance.h"

class VanillaCreationTask final : public InstanceTask {
    Q_OBJECT
   public:
    explicit VanillaCreationTask(BaseVersion::Ptr version) : m_version(std::move(version)) {}
    VanillaCreationTask(BaseVersion::Ptr version, QString loader, BaseVersion::Ptr loaderVersion);

    void executeTask() override;

   private:
    std::unique_ptr<MinecraftInstance> m_instance;

    // Version to update to / create of the instance.
    BaseVersion::Ptr m_version;

    bool m_usingLoader = false;
    QString m_loader;
    BaseVersion::Ptr m_loaderVersion;
};
