#pragma once

#include <optional>

#include <QAbstractListModel>

#include "QObjectPtr.h"
#include "BaseInstance.h"
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
    ResourceModel(BaseInstance const&, ResourceAPI* api);
    ~ResourceModel() override;

    [[nodiscard]] auto data(const QModelIndex&, int role) const -> QVariant override;
    [[nodiscard]] auto roleNames() const -> QHash<int, QByteArray> override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    [[nodiscard]] virtual auto debugName() const -> QString;
    [[nodiscard]] virtual auto metaEntryBase() const -> QString = 0;

    [[nodiscard]] inline int rowCount(const QModelIndex& parent) const override { return parent.isValid() ? 0 : m_packs.size(); }
    [[nodiscard]] inline int columnCount(const QModelIndex& parent) const override { return parent.isValid() ? 0 : 1; };
    [[nodiscard]] inline auto flags(const QModelIndex& index) const -> Qt::ItemFlags override { return QAbstractListModel::flags(index); };

    inline void addActiveJob(Task::Ptr ptr) { m_current_job.addTask(ptr); if (!m_current_job.isRunning()) m_current_job.start(); }
    inline Task const& activeJob() { return m_current_job; }

   signals:
    void versionListUpdated();
    void projectInfoUpdated();

   public slots:
    void fetchMore(const QModelIndex& parent) override;
    [[nodiscard]] inline bool canFetchMore(const QModelIndex& parent) const override
    {
        return parent.isValid() ? false : m_search_state == SearchState::CanFetchMore;
    }

    void setSearchTerm(QString term) { m_search_term = term; }

    virtual ResourceAPI::SearchArgs createSearchArguments() = 0;
    virtual ResourceAPI::SearchCallbacks createSearchCallbacks() = 0;

    virtual ResourceAPI::VersionSearchArgs createVersionsArguments(QModelIndex&) = 0;
    virtual ResourceAPI::VersionSearchCallbacks createVersionsCallbacks(QModelIndex&) = 0;

    virtual ResourceAPI::ProjectInfoArgs createInfoArguments(QModelIndex&) = 0;
    virtual ResourceAPI::ProjectInfoCallbacks createInfoCallbacks(QModelIndex&) = 0;

    /** Requests the API for more entries. */
    virtual void search();

    /** Applies any processing / extra requests needed to fully load the specified entry's information. */
    virtual void loadEntry(QModelIndex&);

    /** Schedule a refresh, clearing the current state. */
    void refresh();

    /** Gets the icon at the URL for the given index. If it's not fetched yet, fetch it and update when fisinhed. */
    std::optional<QIcon> getIcon(QModelIndex&, const QUrl&);

   protected:
    /** Resets the model's data. */
    void clearData();

   protected:
    const BaseInstance& m_base_instance;

    /* Basic search parameters */
    enum class SearchState { None, CanFetchMore, ResetRequested, Finished } m_search_state = SearchState::None;
    int m_next_search_offset = 0;
    QString m_search_term;

    std::unique_ptr<ResourceAPI> m_api;

    ConcurrentTask m_current_job;

    shared_qobject_ptr<NetJob> m_current_icon_job;
    QSet<QUrl> m_currently_running_icon_actions;
    QSet<QUrl> m_failed_icon_actions;

    QList<ModPlatform::IndexedPack> m_packs;

    // HACK: We need this to prevent callbacks from calling the model after it has already been deleted.
    // This leaks a tiny bit of memory per time the user has opened a resource dialog. How to make this better?
    static QHash<ResourceModel*, bool> s_running_models;

   private:
    /* Default search request callbacks */
    void searchRequestFailed(QString reason, int network_error_code);
    void searchRequestAborted();
};

}  // namespace ResourceDownload
