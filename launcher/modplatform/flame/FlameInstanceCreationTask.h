#pragma once

#include "InstanceCreationTask.h"

#include <optional>

#include "minecraft/MinecraftInstance.h"

#include "modplatform/flame/FileResolvingTask.h"

#include "net/NetJob.h"

#include "ui/dialogs/BlockedModsDialog.h"

class FlameCreationTask final : public InstanceCreationTask {
    Q_OBJECT

   public:
    FlameCreationTask(const QString& staging_path, SettingsObjectPtr global_settings, QWidget* parent)
        : InstanceCreationTask(), m_parent(parent)
    {
        setStagingPath(staging_path);
        setParentSettings(global_settings);
    }

    bool abort() override;

    bool updateInstance() override;
    bool createInstance() override;

   private slots:
    void idResolverSucceeded(QEventLoop&);
    void setupDownloadJob(QEventLoop&);
    void copyBlockedMods(QList<BlockedMod> const& blocked_mods);

   private:
    QWidget* m_parent = nullptr;

    shared_qobject_ptr<Flame::FileResolvingTask> m_mod_id_resolver;
    Flame::Manifest m_pack;

    // Handle to allow aborting
    NetJob* m_process_update_file_info_job = nullptr;
    NetJob::Ptr m_files_job = nullptr;

    std::optional<InstancePtr> m_instance;
};
