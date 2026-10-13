#pragma once

#include <optional>

#include <QByteArray>
#include <QCryptographicHash>
#include <QHash>
#include <QQueue>
#include <QString>
#include <QUrl>
#include <QVector>
#include <utility>

#include "BaseInstance.h"
#include "InstanceTask.h"
#include "minecraft/ModLoaderType.h"

class Resource;

class ModrinthCreationTask final : public InstanceTask {
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
                         bool trustedSource,
                         SettingsObject* globalSettings,
                         QWidget* parent,
                         QString id,
                         QString versionId = {},
                         QString originalInstanceId = {});
    ~ModrinthCreationTask() override;

    bool abort() override;

    void createInstance();
    void executeTask() override;

   private slots:
    void finishInstall();

   private:
    bool parseManifest(const QString&, std::vector<File>&, bool setInternalData = true, bool showOptionalDialog = true);

    void ensureMetaLoop();
    void setManagedPack(BaseInstance* instance);

    [[nodiscard]] bool promptForUntrustedMods();

   private:
    QWidget* m_parent = nullptr;
    bool m_trustedSource;

    QString m_minecraftVersion;
    QList<std::pair<ModLoaderType, QString>> m_loaderVersions;
    QString m_managedId, m_managedVersionId, m_managedName;

    std::vector<File> m_files;
    Task::Ptr m_task;

    std::optional<BaseInstance*> m_oldInstance;
    std::unique_ptr<MinecraftInstance> m_newInstance;

    QString m_rootPath = "minecraft";

    QHash<QString, Resource*> m_resources;
};
