#pragma once

#include <optional>

#include <QByteArray>
#include <QCryptographicHash>
#include <QQueue>
#include <QString>
#include <QUrl>
#include <QVector>

#include "BaseInstance.h"
#include "InstanceCreationTask.h"

class ModrinthCreationTask final : public InstanceCreationTask {
    Q_OBJECT
    struct File {
        QString path;

        QCryptographicHash::Algorithm hashAlgorithm;
        QByteArray hash;
        QQueue<QUrl> downloads;
        bool required = true;
    };

   public:
    ModrinthCreationTask(const QString& stagingPath,
                         SettingsObject* globalSettings,
                         QWidget* parent,
                         QString id,
                         QString versionId = {},
                         QString originalInstanceId = {})
        : m_parent(parent), m_managed_id(std::move(id)), m_managed_version_id(std::move(versionId))
    {
        setStagingPath(stagingPath);
        setParentSettings(globalSettings);

        m_original_instance_id = std::move(originalInstanceId);
    }

    bool abort() override;

    bool updateInstance() override;
    std::unique_ptr<MinecraftInstance> createInstance() override;

   private:
    bool parseManifest(const QString&, std::vector<File>&, bool setInternalData = true, bool showOptionalDialog = true);

   private:
    QWidget* m_parent = nullptr;

    QString m_minecraft_version, m_fabric_version, m_quilt_version, m_forge_version, m_neoForge_version;
    QString m_managed_id, m_managed_version_id, m_managed_name;

    std::vector<File> m_files;
    Task::Ptr m_task;

    std::optional<BaseInstance*> m_instance;

    QString m_root_path = "minecraft";
};
