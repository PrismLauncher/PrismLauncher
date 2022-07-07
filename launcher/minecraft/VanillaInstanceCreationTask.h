#pragma once

#include "InstanceCreationTask.h"

class VanillaCreationTask final : public InstanceCreationTask {
    Q_OBJECT
   public:
    VanillaCreationTask(BaseVersionPtr version) : InstanceCreationTask(), m_version(version) {}
    VanillaCreationTask(BaseVersionPtr version, QString loader, BaseVersionPtr loader_version);

    bool createInstance() override;

   private:
    // Version to update to / create of the instance.
    BaseVersionPtr m_version;

    bool m_using_loader = false;
    QString m_loader;
    BaseVersionPtr m_loader_version;
};
