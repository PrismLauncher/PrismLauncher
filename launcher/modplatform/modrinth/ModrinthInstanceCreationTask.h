#pragma once

#include "InstanceCreationTask.h"

#include "modplatform/modrinth/ModrinthPackManifest.h"

#include "net/NetJob.h"

class ModrinthCreationTask final : public InstanceCreationTask {
    Q_OBJECT

   public:
    ModrinthCreationTask(QString staging_path, SettingsObjectPtr global_settings, QWidget* parent, QString source_url = {})
        : InstanceCreationTask(), m_parent(parent)
    {
        setStagingPath(staging_path);
        setParentSettings(global_settings);
    }

    bool abort() override;
    bool canAbort() const override { return true; }

    bool updateInstance() override;
    bool createInstance() override;

   private:
    bool parseManifest(QString, std::vector<Modrinth::File>&);
    QString getManagedPackID() const;

   private:
    QWidget* m_parent = nullptr;

    QString minecraftVersion, fabricVersion, quiltVersion, forgeVersion;
    QString m_managed_id, m_managed_version_id, m_managed_name;
    QString m_source_url;

    std::vector<Modrinth::File> m_files;
    NetJob::Ptr m_files_job;
};
