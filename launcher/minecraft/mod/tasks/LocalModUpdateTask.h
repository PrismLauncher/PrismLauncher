#pragma once

#include <QDir>

#include "modplatform/ModIndex.h"
#include "tasks/Task.h"

class LocalModUpdateTask : public Task {
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<LocalModUpdateTask>;

    explicit LocalModUpdateTask(QDir mods_dir, ModPlatform::IndexedPack& mod, ModPlatform::IndexedVersion& mod_version);

    auto canAbort() const -> bool override { return true; }
    auto abort() -> bool override;

   protected slots:
    //! Entry point for tasks.
    void executeTask() override;

   private:
    QDir m_index_dir;
    ModPlatform::IndexedPack& m_mod;
    ModPlatform::IndexedVersion& m_mod_version;
};
