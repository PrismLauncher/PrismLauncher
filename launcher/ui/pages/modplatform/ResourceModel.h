// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <optional>

#include <QAbstractListModel>

#include "QObjectPtr.h"

#include "ResourceDownloadTask.h"
#include "modplatform/ResourceAPI.h"

#include "tasks/ConcurrentTask.h"

class NetJob;
class ResourceAPI;

namespace ModPlatform {
struct IndexedPack;
}

namespace ResourceDownload {

class ResourceModel : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(QString search_term MEMBER m_search_term WRITE setSearchTerm)

   public:
    using DownloadTaskPtr = shared_qobject_ptr<ResourceDownloadTask>;

    ResourceModel(ResourceAPI* api);
    ~ResourceModel() override;

    [[nodiscard]] auto data(const QModelIndex&, int role) const -> QVariant override;
    [[nodiscard]] auto roleNames() const -> QHash<int, QByteArray> override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    [[nodiscard]] virtual auto debugName() const -> QString;
    [[nodiscard]] virtual auto metaEntryBase() const -> QString = 0;

    [[nodiscard]] inline int rowCount(const QModelIndex& parent) const override
    {
        return parent.isValid() ? 0 : static_cast<int>(m_packs.size());
    }
    [[nodiscard]] inline int columnCount(const QModelIndex& parent) const override { return parent.isValid() ? 0 : 1; }
    [[nodiscard]] inline auto flags(const QModelIndex& index) const -> Qt::ItemFlags override { return QAbstractListModel::flags(index); }

    [[nodiscard]] bool hasActiveSearchJob() const { return m_current_search_job && m_current_search_job->isRunning(); }
    [[nodiscard]] bool hasActiveInfoJob() const { return m_current_info_job.isRunning(); }
    [[nodiscard]] Task::Ptr activeSearchJob() { return hasActiveSearchJob() ? m_current_search_job : nullptr; }

    [[nodiscard]] auto getSortingMethods() const { return m_api->getSortingMethods(); }

   public slots:
    void fetchMore(const QModelIndex& parent) override;
    // NOTE: Can't use [[nodiscard]] here because of https://bugreports.qt.io/browse/QTBUG-58628 on Qt 5.12
    inline bool canFetchMore(const QModelIndex& parent) const override
    {
        return parent.isValid() ? false : m_search_state == SearchState::CanFetchMore;
    }

    void setSearchTerm(QString term) { m_search_term = term; }

    virtual ResourceAPI::SearchArgs createSearchArguments() = 0;
    virtual ResourceAPI::SearchCallbacks createSearchCallbacks() { return {}; }

    virtual ResourceAPI::VersionSearchArgs createVersionsArguments(QModelIndex&) = 0;
    virtual ResourceAPI::VersionSearchCallbacks createVersionsCallbacks(QModelIndex&) { return {}; }

    virtual ResourceAPI::ProjectInfoArgs createInfoArguments(QModelIndex&) = 0;
    virtual ResourceAPI::ProjectInfoCallbacks createInfoCallbacks(QModelIndex&) { return {}; }

    /** Requests the API for more entries. */
    virtual void search();

    /** Applies any processing / extra requests needed to fully load the specified entry's information. */
    virtual void loadEntry(QModelIndex&);

    /** Schedule a refresh, clearing the current state. */
    void refresh();

    /** Gets the icon at the URL for the given index. If it's not fetched yet, fetch it and update when fisinhed. */
    std::optional<QIcon> getIcon(QModelIndex&, const QUrl&);

    void addPack(ModPlatform::IndexedPack::Ptr pack,
                 ModPlatform::IndexedVersion& version,
                 const std::shared_ptr<ResourceFolderModel> packs,
                 bool is_indexed = false,
                 QString custom_target_folder = {});
    void removePack(const QString& rem);
    QList<DownloadTaskPtr> selectedPacks() { return m_selected; }

   protected:
    /** Resets the model's data. */
    void clearData();

    void runSearchJob(Task::Ptr);
    void runInfoJob(Task::Ptr);

    [[nodiscard]] auto getCurrentSortingMethodByIndex() const -> std::optional<ResourceAPI::SortingMethod>;

    /** Converts a JSON document to a common array format.
     *
     *  This is needed so that different providers, with different JSON structures, can be parsed
     *  uniformally. You NEED to re-implement this if you intend on using the default callbacks.
     */
    [[nodiscard]] virtual auto documentToArray(QJsonDocument&) const -> QJsonArray;

    /** Functions to load data into a pack.
     *
     *  Those are needed for the same reason as documentToArray, and NEED to be re-implemented in the same way.
     */

    virtual void loadIndexedPack(ModPlatform::IndexedPack&, QJsonObject&);
    virtual void loadExtraPackInfo(ModPlatform::IndexedPack&, QJsonObject&);
    virtual void loadIndexedPackVersions(ModPlatform::IndexedPack&, QJsonArray&);

    virtual bool isPackInstalled(ModPlatform::IndexedPack::Ptr) const { return false; }

   protected:
    /* Basic search parameters */
    enum class SearchState { None, CanFetchMore, ResetRequested, Finished } m_search_state = SearchState::None;
    int m_next_search_offset = 0;
    QString m_search_term;
    unsigned int m_current_sort_index = 0;

    std::unique_ptr<ResourceAPI> m_api;

    // Job for searching for new entries
    shared_qobject_ptr<Task> m_current_search_job;
    // Job for fetching versions and extra info on existing entries
    ConcurrentTask m_current_info_job;

    shared_qobject_ptr<NetJob> m_current_icon_job;
    QSet<QUrl> m_currently_running_icon_actions;
    QSet<QUrl> m_failed_icon_actions;

    QList<ModPlatform::IndexedPack::Ptr> m_packs;
    QList<DownloadTaskPtr> m_selected;

    // HACK: We need this to prevent callbacks from calling the model after it has already been deleted.
    // This leaks a tiny bit of memory per time the user has opened a resource dialog. How to make this better?
    static QHash<ResourceModel*, bool> s_running_models;

   private:
    /* Default search request callbacks */
    void searchRequestSucceeded(QJsonDocument&);
    void searchRequestForOneSucceeded(QJsonDocument&);
    void searchRequestFailed(QString reason, int network_error_code);
    void searchRequestAborted();

    void versionRequestSucceeded(QJsonDocument&, ModPlatform::IndexedPack&, const QModelIndex&);

    void infoRequestSucceeded(QJsonDocument&, ModPlatform::IndexedPack&, const QModelIndex&);

   signals:
    void versionListUpdated();
    void projectInfoUpdated();
};

}  // namespace ResourceDownload
