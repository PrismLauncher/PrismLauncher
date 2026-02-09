#pragma once

#include "BaseInstance.h"
#include "ResourceDownloadTask.h"
#include "ReviewMessageBox.h"

#include "minecraft/mod/ModFolderModel.h"

#include "modplatform/CheckUpdateTask.h"

class Mod;
class ModrinthCheckUpdate;
class FlameCheckUpdate;
class ConcurrentTask;
class QTreeWidgetItem;

class ResourceUpdateDialog final : public ReviewMessageBox {
    Q_OBJECT
   public:
    explicit ResourceUpdateDialog(QWidget* parent,
                                  BaseInstance* instance,
                                  ResourceFolderModel* resourceModel,
                                  QList<Resource*>& searchFor,
                                  bool includeDeps,
                                  QList<ModPlatform::ModLoaderType> loadersList = {},
                                  bool defaultUncheckManagedGroups = false);

    void checkCandidates();

    void appendResource(const CheckUpdateTask::Update& info, QStringList requiredBy = {});

    const QList<ResourceDownloadTask::Ptr> getTasks();
    auto indexDir() const -> QDir { return m_resourceModel->indexDir(); }

    auto noUpdates() const -> bool { return m_noUpdates; };
    auto aborted() const -> bool { return m_aborted; };

   private:
    auto ensureMetadata() -> bool;

   private slots:
    void onMetadataEnsured(Resource* resource);
    void onMetadataFailed(Resource* resource,
                          bool try_others = false,
                          ModPlatform::ResourceProvider firstChoice = ModPlatform::ResourceProvider::MODRINTH);

   private:
    QTreeWidgetItem* ensureGroupItem(const QString& groupId);
    QString groupIdForUpdate(const CheckUpdateTask::Update& info) const;
    bool isManagedGroup(const QString& groupId) const;
    QString groupLabel(const QString& groupId) const;

   private:
    struct GroupState {
        QString label;
        bool managedPack = false;
    };

    QWidget* m_parent;

    shared_qobject_ptr<ModrinthCheckUpdate> m_modrinthCheckTask;
    shared_qobject_ptr<FlameCheckUpdate> m_flameCheckTask;

    ResourceFolderModel* m_resourceModel;

    QList<Resource*>& m_candidates;
    QList<Resource*> m_modrinthToUpdate;
    QList<Resource*> m_flameToUpdate;

    ConcurrentTask::Ptr m_secondTryMetadata;
    QList<std::tuple<Resource*, QString>> m_failedMetadata;
    QList<std::tuple<Resource*, QString, QUrl>> m_failedCheckUpdate;

    QHash<QString, ResourceDownloadTask::Ptr> m_tasks;
    QHash<QTreeWidgetItem*, ResourceDownloadTask::Ptr> m_itemTasks;
    QHash<QString, QTreeWidgetItem*> m_groupItems;
    QHash<QString, GroupState> m_groupState;
    ModFolderModel* m_modModel = nullptr;
    BaseInstance* m_instance;

    bool m_noUpdates = false;
    bool m_aborted = false;
    bool m_includeDeps = false;
    bool m_defaultUncheckManagedGroups = false;
    bool m_groupedViewEnabled = false;
    QList<ModPlatform::ModLoaderType> m_loadersList;
};
