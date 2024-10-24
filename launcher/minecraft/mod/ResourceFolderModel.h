#pragma once

#include <QAbstractListModel>
#include <QAction>
#include <QDir>
#include <QFileSystemWatcher>
#include <QHeaderView>
#include <QMutex>
#include <QSet>
#include <QSortFilterProxyModel>
#include <QTreeView>

#include "Resource.h"

#include "BaseInstance.h"

#include "tasks/ConcurrentTask.h"
#include "tasks/Task.h"

class QSortFilterProxyModel;

/* A macro to define useful functions to handle Resource* -> T* more easily on derived classes */
#define RESOURCE_HELPERS(T)                                                                       \
    [[nodiscard]] T& operator[](int index)                                                        \
    {                                                                                             \
        return *static_cast<T*>(m_resources[index].get());                                        \
    }                                                                                             \
    [[nodiscard]] T& at(int index)                                                                \
    {                                                                                             \
        return *static_cast<T*>(m_resources[index].get());                                        \
    }                                                                                             \
    [[nodiscard]] const T& at(int index) const                                                    \
    {                                                                                             \
        return *static_cast<const T*>(m_resources.at(index).get());                               \
    }                                                                                             \
    [[nodiscard]] T& first()                                                                      \
    {                                                                                             \
        return *static_cast<T*>(m_resources.first().get());                                       \
    }                                                                                             \
    [[nodiscard]] T& last()                                                                       \
    {                                                                                             \
        return *static_cast<T*>(m_resources.last().get());                                        \
    }                                                                                             \
    [[nodiscard]] T* find(QString id)                                                             \
    {                                                                                             \
        auto iter = std::find_if(m_resources.constBegin(), m_resources.constEnd(),                \
                                 [&](Resource::Ptr const& r) { return r->internal_id() == id; }); \
        if (iter == m_resources.constEnd())                                                       \
            return nullptr;                                                                       \
        return static_cast<T*>((*iter).get());                                                    \
    }                                                                                             \
    QList<T*> selected##T##s(const QModelIndexList& indexes)                                      \
    {                                                                                             \
        QList<T*> result;                                                                         \
        for (const QModelIndex& index : indexes) {                                                \
            if (index.column() != 0)                                                              \
                continue;                                                                         \
                                                                                                  \
            result.append(&at(index.row()));                                                      \
        }                                                                                         \
        return result;                                                                            \
    }                                                                                             \
    QList<T*> all##T##s()                                                                         \
    {                                                                                             \
        QList<T*> result;                                                                         \
        result.reserve(m_resources.size());                                                       \
                                                                                                  \
        for (const Resource::Ptr& resource : m_resources)                                         \
            result.append(static_cast<T*>(resource.get()));                                       \
                                                                                                  \
        return result;                                                                            \
    }

/** A basic model for external resources.
 *
 *  This model manages a list of resources. As such, external users of such resources do not own them,
 *  and the resource's lifetime is contingent on the model's lifetime.
 *
 *  TODO: Make the resources unique pointers accessible through weak pointers.
 */
class ResourceFolderModel : public QAbstractListModel {
    Q_OBJECT
   public:
    ResourceFolderModel(const QDir& dir, BaseInstance* instance, bool is_indexed, bool create_dir, QObject* parent = nullptr);
    ~ResourceFolderModel() override;

    virtual QString id() const { return "resource"; }

    /** Starts watching the paths for changes.
     *
     *  Returns whether starting to watch all the paths was successful.
     *  If one or more fails, it returns false.
     */
    bool startWatching(const QStringList& paths);

    /** Stops watching the paths for changes.
     *
     *  Returns whether stopping to watch all the paths was successful.
     *  If one or more fails, it returns false.
     */
    bool stopWatching(const QStringList& paths);

    /* Helper methods for subclasses, using a predetermined list of paths. */
    virtual bool startWatching() { return startWatching({ indexDir().absolutePath(), m_dir.absolutePath() }); }
    virtual bool stopWatching() { return stopWatching({ indexDir().absolutePath(), m_dir.absolutePath() }); }

    QDir indexDir() { return { QString("%1/.index").arg(dir().absolutePath()) }; }

    /** Given a path in the system, install that resource, moving it to its place in the
     *  instance file hierarchy.
     *
     *  Returns whether the installation was succcessful.
     */
    virtual bool installResource(QString path);

    virtual bool installResource(QString path, ModPlatform::IndexedVersion& vers);

    /** Uninstall (i.e. remove all data about it) a resource, given its file name.
     *
     *  Returns whether the removal was successful.
     */
    virtual bool uninstallResource(QString file_name, bool preserve_metadata = false);
    virtual bool deleteResources(const QModelIndexList&);
    virtual void deleteMetadata(const QModelIndexList&);

    /** Applies the given 'action' to the resources in 'indexes'.
     *
     *  Returns whether the action was successfully applied to all resources.
     */
    virtual bool setResourceEnabled(const QModelIndexList& indexes, EnableAction action);

    /** Creates a new update task and start it. Returns false if no update was done, like when an update is already underway. */
    virtual bool update();

    /** Creates a new parse task, if needed, for 'res' and start it.*/
    virtual void resolveResource(Resource* res);

    [[nodiscard]] qsizetype size() const { return m_resources.size(); }
    [[nodiscard]] bool empty() const { return size() == 0; }
    RESOURCE_HELPERS(Resource)

    [[nodiscard]] QDir const& dir() const { return m_dir; }

    /** Checks whether there's any parse tasks being done.
     *
     *  Since they can be quite expensive, and are usually done in a separate thread, if we were to destroy the model while having
     *  such tasks would introduce an undefined behavior, most likely resulting in a crash.
     */
    [[nodiscard]] bool hasPendingParseTasks() const;

    /* Qt behavior */

    /* Basic columns */
    enum Columns { ACTIVE_COLUMN = 0, NAME_COLUMN, DATE_COLUMN, PROVIDER_COLUMN, SIZE_COLUMN, NUM_COLUMNS };

    QStringList columnNames(bool translated = true) const { return translated ? m_column_names_translated : m_column_names; }

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override { return parent.isValid() ? 0 : static_cast<int>(size()); }
    [[nodiscard]] int columnCount(const QModelIndex& parent = {}) const override { return parent.isValid() ? 0 : NUM_COLUMNS; }

    [[nodiscard]] Qt::DropActions supportedDropActions() const override;

    /// flags, mostly to support drag&drop
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
    [[nodiscard]] QStringList mimeTypes() const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

    [[nodiscard]] bool validateIndex(const QModelIndex& index) const;

    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void setupHeaderAction(QAction* act, int column);
    void saveColumns(QTreeView* tree);
    void loadColumns(QTreeView* tree);
    QMenu* createHeaderContextMenu(QTreeView* tree);

    /** This creates a proxy model to filter / sort the model for a UI.
     *
     *  The actual comparisons and filtering are done directly by the Resource, so to modify behavior go there instead!
     */
    QSortFilterProxyModel* createFilterProxyModel(QObject* parent = nullptr);

    [[nodiscard]] SortType columnToSortKey(size_t column) const;
    [[nodiscard]] QList<QHeaderView::ResizeMode> columnResizeModes() const { return m_column_resize_modes; }

    class ProxyModel : public QSortFilterProxyModel {
       public:
        explicit ProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

       protected:
        [[nodiscard]] bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
        [[nodiscard]] bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;
    };

    QString instDirPath() const;

   signals:
    void updateFinished();
    void parseFinished();

   protected:
    /** This creates a new update task to be executed by update().
     *
     *  The task should load and parse all resources necessary, and provide a way of accessing such results.
     *
     *  This Task is normally executed when opening a page, so it shouldn't contain much heavy work.
     *  If such work is needed, try using it in the Task create by createParseTask() instead!
     */
    [[nodiscard]] Task* createUpdateTask();

    [[nodiscard]] virtual Resource* createResource(const QFileInfo& info) { return new Resource(info); }

    /** This creates a new parse task to be executed by onUpdateSucceeded().
     *
     *  This task should load and parse all heavy info needed by a resource, such as parsing a manifest. It gets executed
     *  in the background, so it slowly updates the UI as tasks get done.
     */
    [[nodiscard]] virtual Task* createParseTask(Resource&) { return nullptr; }

    /** Standard implementation of the model update logic.
     *
     *  It uses set operations to find differences between the current state and the updated state,
     *  to act only on those disparities.
     *
     *  The implementation is at the end of this header.
     */
    template <typename T>
    void applyUpdates(QSet<QString>& current_set, QSet<QString>& new_set, QMap<QString, T>& new_resources);

   protected slots:
    void directoryChanged(QString);

    /** Called when the update task is successful.
     *
     *  This usually calls static_cast on the specific Task type returned by createUpdateTask,
     *  so care must be taken in such cases.
     *  TODO: Figure out a way to express this relationship better without templated classes (Q_OBJECT macro disallows that).
     */
    virtual void onUpdateSucceeded();
    virtual void onUpdateFailed() {}

    /** Called when the parse task with the given ticket is successful.
     *
     *  This is just a simple reference implementation. You probably want to override it with your own logic in a subclass
     *  if the resource is complex and has more stuff to parse.
     */
    virtual void onParseSucceeded(int ticket, QString resource_id);
    virtual void onParseFailed(int ticket, QString resource_id);

   protected:
    // Represents the relationship between a column's index (represented by the list index), and it's sorting key.
    // As such, the order in with they appear is very important!
    QList<SortType> m_column_sort_keys = { SortType::ENABLED, SortType::NAME, SortType::DATE, SortType::PROVIDER, SortType::SIZE };
    QStringList m_column_names = { "Enable", "Name", "Last Modified", "Provider", "Size" };
    QStringList m_column_names_translated = { tr("Enable"), tr("Name"), tr("Last Modified"), tr("Provider"), tr("Size") };
    QList<QHeaderView::ResizeMode> m_column_resize_modes = { QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Stretch, QHeaderView::Interactive,
                                                             QHeaderView::Interactive };
    QList<bool> m_columnsHideable = { false, false, true, true, true };
    QList<bool> m_columnsHiddenByDefault = { false, false, false, false, true };

    QDir m_dir;
    BaseInstance* m_instance;
    QFileSystemWatcher m_watcher;
    bool m_is_watching = false;

    bool m_is_indexed;
    bool m_first_folder_load = true;

    Task::Ptr m_current_update_task = nullptr;
    bool m_scheduled_update = false;

    QList<Resource::Ptr> m_resources;

    // Represents the relationship between a resource's internal ID and it's row position on the model.
    QMap<QString, int> m_resources_index;

    ConcurrentTask m_helper_thread_task;
    QMap<int, Task::Ptr> m_active_parse_tasks;
    std::atomic<int> m_next_resolution_ticket = 0;
};

/* Template definition to avoid some code duplication */
template <typename T>
void ResourceFolderModel::applyUpdates(QSet<QString>& current_set, QSet<QString>& new_set, QMap<QString, T>& new_resources)
{
    // see if the kept resources changed in some way
    {
        QSet<QString> kept_set = current_set;
        kept_set.intersect(new_set);

        for (auto const& kept : kept_set) {
            auto row_it = m_resources_index.constFind(kept);
            Q_ASSERT(row_it != m_resources_index.constEnd());
            auto row = row_it.value();

            auto& new_resource = new_resources[kept];
            auto const& current_resource = m_resources.at(row);

            if (new_resource->dateTimeChanged() == current_resource->dateTimeChanged()) {
                // no significant change, ignore...
                continue;
            }

            // If the resource is resolving, but something about it changed, we don't want to
            // continue the resolving.
            if (current_resource->isResolving()) {
                auto ticket = current_resource->resolutionTicket();
                if (m_active_parse_tasks.contains(ticket)) {
                    auto task = (*m_active_parse_tasks.find(ticket)).get();
                    task->abort();
                }
            }

            m_resources[row].reset(new_resource);
            resolveResource(m_resources.at(row).get());
            emit dataChanged(index(row, 0), index(row, columnCount(QModelIndex()) - 1));
        }
    }

    // remove resources no longer present
    {
        QSet<QString> removed_set = current_set;
        removed_set.subtract(new_set);

        QList<int> removed_rows;
        for (auto& removed : removed_set)
            removed_rows.append(m_resources_index[removed]);

        std::sort(removed_rows.begin(), removed_rows.end(), std::greater<int>());

        for (auto& removed_index : removed_rows) {
            auto removed_it = m_resources.begin() + removed_index;

            Q_ASSERT(removed_it != m_resources.end());

            if ((*removed_it)->isResolving()) {
                auto ticket = (*removed_it)->resolutionTicket();
                if (m_active_parse_tasks.contains(ticket)) {
                    auto task = (*m_active_parse_tasks.find(ticket)).get();
                    task->abort();
                }
            }

            beginRemoveRows(QModelIndex(), removed_index, removed_index);
            m_resources.erase(removed_it);
            endRemoveRows();
        }
    }

    // add new resources to the end
    {
        QSet<QString> added_set = new_set;
        added_set.subtract(current_set);

        // When you have a Qt build with assertions turned on, proceeding here will abort the application
        if (added_set.size() > 0) {
            beginInsertRows(QModelIndex(), static_cast<int>(m_resources.size()),
                            static_cast<int>(m_resources.size() + added_set.size() - 1));

            for (auto& added : added_set) {
                auto res = new_resources[added];
                m_resources.append(res);
                resolveResource(m_resources.last().get());
            }

            endInsertRows();
        }
    }

    // update index
    {
        m_resources_index.clear();
        int idx = 0;
        for (auto const& mod : qAsConst(m_resources)) {
            m_resources_index[mod->internal_id()] = idx;
            idx++;
        }
    }
}
