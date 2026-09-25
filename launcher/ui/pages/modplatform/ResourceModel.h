// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <optional>

#include <QAbstractListModel>
#include <utility>

#include "QObjectPtr.h"

#include "ResourceDownloadTask.h"
#include "modplatform/ModIndex.h"
#include "modplatform/ResourceAPI.h"

#include "tasks/ConcurrentTask.h"

class NetJob;
class ResourceAPI;
class ResourceFolderModel;

namespace ModPlatform {
struct IndexedPack;
}

namespace ResourceDownload {

class ResourceModel : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(QString search_term MEMBER m_searchTerm WRITE setSearchTerm)

   public:
    using DownloadTaskPtr = shared_qobject_ptr<ResourceDownloadTask>;

    ResourceModel(ResourceFolderModel*, const ResourceAPI* api);
    ~ResourceModel() override;

    auto data(const QModelIndex& /*index*/, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    virtual auto debugName() const -> QString;
    virtual auto metaEntryBase() const -> QString = 0;

    int rowCount(const QModelIndex& parent) const override { return parent.isValid() ? 0 : static_cast<int>(m_packs.size()); }
    int columnCount(const QModelIndex& parent) const override { return parent.isValid() ? 0 : 1; }
    auto flags(const QModelIndex& index) const -> Qt::ItemFlags override { return QAbstractListModel::flags(index); }

    bool hasActiveSearchJob() const { return m_currentSearchJob && m_currentSearchJob->isRunning(); }
    bool hasActiveInfoJob() const { return m_currentInfoJob.isRunning(); }
    Task::Ptr activeSearchJob() { return hasActiveSearchJob() ? m_currentSearchJob : nullptr; }

    auto getSortingMethods() const { return m_api->getSortingMethods(); }

    virtual QVariant getInstalledPackVersion(ModPlatform::IndexedPack::Ptr /*unused*/) const;
    /** Whether the version is opted out or not. Currently only makes sense in CF. */
    virtual bool optedOut(const ModPlatform::IndexedVersion& ver) const
    {
        Q_UNUSED(ver);
        return false;
    };

    virtual bool checkFilters(ModPlatform::IndexedPack::Ptr /*unused*/) { return true; }
    virtual bool checkVersionFilters(const ModPlatform::IndexedVersion&);

   public slots:
    void fetchMore(const QModelIndex& parent) override;
    bool canFetchMore(const QModelIndex& parent) const override
    {
        return parent.isValid() ? false : m_searchState == SearchState::CanFetchMore;
    }

    void setSearchTerm(QString term) { m_searchTerm = std::move(term); }

    virtual ResourceAPI::SearchArgs createSearchArguments() = 0;

    virtual ResourceAPI::VersionSearchArgs createVersionsArguments(const QModelIndex&) = 0;

    virtual ResourceAPI::ProjectInfoArgs createInfoArguments(const QModelIndex&) = 0;

    /** Requests the API for more entries. */
    virtual void search();

    /** Applies any processing / extra requests needed to fully load the specified entry's information. */
    virtual void loadEntry(const QModelIndex&);

    /** Schedule a refresh, clearing the current state. */
    void refresh();

    /** Gets the icon at the URL for the given index. If it's not fetched yet, fetch it and update when fisinhed. */
    std::optional<QIcon> getIcon(const QModelIndex&, const QUrl&);

    void addPack(ModPlatform::IndexedPack::Ptr pack,
                 ModPlatform::IndexedVersion& version,
                 ResourceFolderModel* packs,
                 bool isIndexed = false,
                 QString downloadReason = "standalone",
                 QString dependentOn = "");
    void removePack(const QString& rem);
    QList<DownloadTaskPtr> selectedPacks() { return m_selected; }

   protected:
    /** Resets the model's data. */
    void clearData();

    void runSearchJob(const Task::Ptr&);
    void runInfoJob(Task::Ptr);

    auto getCurrentSortingMethodByIndex() const -> std::optional<ResourceAPI::SortingMethod>;

    virtual bool isPackInstalled(ModPlatform::IndexedPack::Ptr /*unused*/) const;

   protected:
    /* Basic search parameters */
    enum class SearchState : std::uint8_t { None, CanFetchMore, ResetRequested, Finished } m_searchState = SearchState::None;
    int m_nextSearchOffset = 0;
    QString m_searchTerm;
    unsigned int m_currentSortIndex = 0;

    ResourceFolderModel* m_resourceList = nullptr;

    const ResourceAPI* m_api;

    // Job for searching for new entries
    shared_qobject_ptr<Task> m_currentSearchJob;
    // Job for fetching versions and extra info on existing entries
    ConcurrentTask m_currentInfoJob;

    shared_qobject_ptr<NetJob> m_currentIconJob;
    QSet<QUrl> m_currentlyRunningIconActions;
    QSet<QUrl> m_failedIconActions;

    QList<ModPlatform::IndexedPack::Ptr> m_packs;
    QList<DownloadTaskPtr> m_selected;

    // HACK: We need this to prevent callbacks from calling the model after it has already been deleted.
    // This leaks a tiny bit of memory per time the user has opened a resource dialog. How to make this better?
    static QHash<ResourceModel*, bool> s_runningModels;

   private:
    /* Default search request callbacks */
    void searchRequestSucceeded(QList<ModPlatform::IndexedPack::Ptr>&);
    void searchRequestForOneSucceeded(const ModPlatform::IndexedPack::Ptr&);
    void searchRequestFailed(const QString& reason, int networkErrorCode);
    void searchRequestAborted();

    void versionRequestSucceeded(QVector<ModPlatform::IndexedVersion>&, const QVariant&, const QModelIndex&);

    void infoRequestSucceeded(ModPlatform::IndexedPack::Ptr, const QModelIndex&);

   signals:
    void versionListUpdated(const QModelIndex& index);
    void projectInfoUpdated(const QModelIndex& index);
};

}  // namespace ResourceDownload
