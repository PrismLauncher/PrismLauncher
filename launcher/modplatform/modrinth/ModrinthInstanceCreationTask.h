#pragma once

#include "InstanceCreationTask.h"

#include <optional>

#include "minecraft/MinecraftInstance.h"

#include "modplatform/modrinth/ModrinthPackManifest.h"

#include "net/NetJob.h"

class ModrinthCreationTask final : public InstanceCreationTask {
    Q_OBJECT

   public:
    ModrinthCreationTask(QString staging_path,
                         SettingsObjectPtr global_settings,
                         QWidget* parent,
                         QString id,
                         QString version_id = {},
                         QString original_instance_id = {})
        : InstanceCreationTask(), m_parent(parent), m_managed_id(std::move(id)), m_managed_version_id(std::move(version_id))
    {
        setStagingPath(staging_path);
        setParentSettings(global_settings);

        m_original_instance_id = std::move(original_instance_id);
    }

    bool abort() override;

    void checkUpdate() override;
    void createInstance() override;

    bool canRetry() const override;

   public slots:
    virtual void retry() override
    {
        if (canRetry())
            m_current_task->retry();
    };

   private slots:
    void overrideInstance(InstancePtr);

   private:
    bool parseManifest(const QString&, std::vector<Modrinth::File>&, bool set_internal_data = true, bool show_optional_dialog = true);

   private:
    QWidget* m_parent = nullptr;

    QString m_minecraft_version, m_fabric_version, m_quilt_version, m_forge_version;
    QString m_managed_id, m_managed_version_id, m_managed_name;

    std::vector<Modrinth::File> m_files;

    std::optional<InstancePtr> m_instance;

    Task::Ptr m_current_task;
};
