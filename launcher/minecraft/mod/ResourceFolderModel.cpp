#include "ResourceFolderModel.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QIcon>
#include <QMimeData>
#include <QStyle>
#include <QThreadPool>
#include <QUrl>

#include "Application.h"
#include "FileSystem.h"

#include "minecraft/mod/tasks/BasicFolderLoadTask.h"

#include "tasks/Task.h"

ResourceFolderModel::ResourceFolderModel(QDir dir, BaseInstance* instance, QObject* parent, bool create_dir)
    : QAbstractListModel(parent), m_dir(dir), m_instance(instance), m_watcher(this)
{
    if (create_dir) {
        FS::ensureFolderPathExists(m_dir.absolutePath());
    }

    m_dir.setFilter(QDir::Readable | QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
    m_dir.setSorting(QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &ResourceFolderModel::directoryChanged);
    connect(&m_helper_thread_task, &ConcurrentTask::finished, this, [this] { m_helper_thread_task.clear(); });
}

ResourceFolderModel::~ResourceFolderModel()
{
    while (!QThreadPool::globalInstance()->waitForDone(100))
        QCoreApplication::processEvents();
}

bool ResourceFolderModel::startWatching(const QStringList paths)
{
    if (m_is_watching)
        return false;

    auto couldnt_be_watched = m_watcher.addPaths(paths);
    for (auto path : paths) {
        if (couldnt_be_watched.contains(path))
            qDebug() << "Failed to start watching " << path;
        else
            qDebug() << "Started watching " << path;
    }

    update();

    m_is_watching = !m_is_watching;
    return m_is_watching;
}

bool ResourceFolderModel::stopWatching(const QStringList paths)
{
    if (!m_is_watching)
        return false;

    auto couldnt_be_stopped = m_watcher.removePaths(paths);
    for (auto path : paths) {
        if (couldnt_be_stopped.contains(path))
            qDebug() << "Failed to stop watching " << path;
        else
            qDebug() << "Stopped watching " << path;
    }

    m_is_watching = !m_is_watching;
    return !m_is_watching;
}

bool ResourceFolderModel::installResource(QString original_path)
{
    if (!m_can_interact) {
        return false;
    }

    // NOTE: fix for GH-1178: remove trailing slash to avoid issues with using the empty result of QFileInfo::fileName
    original_path = FS::NormalizePath(original_path);
    QFileInfo file_info(original_path);

    if (!file_info.exists() || !file_info.isReadable()) {
        qWarning() << "Caught attempt to install non-existing file or file-like object:" << original_path;
        return false;
    }
    qDebug() << "Installing: " << file_info.absoluteFilePath();

    Resource resource(file_info);
    if (!resource.valid()) {
        qWarning() << original_path << "is not a valid resource. Ignoring it.";
        return false;
    }

    auto new_path = FS::NormalizePath(m_dir.filePath(file_info.fileName()));
    if (original_path == new_path) {
        qWarning() << "Overwriting the mod (" << original_path << ") with itself makes no sense...";
        return false;
    }

    switch (resource.type()) {
        case ResourceType::SINGLEFILE:
        case ResourceType::ZIPFILE:
        case ResourceType::LITEMOD: {
            if (QFile::exists(new_path) || QFile::exists(new_path + QString(".disabled"))) {
                if (!QFile::remove(new_path)) {
                    qCritical() << "Cleaning up new location (" << new_path << ") was unsuccessful!";
                    return false;
                }
                qDebug() << new_path << "has been deleted.";
            }

            if (!QFile::copy(original_path, new_path)) {
                qCritical() << "Copy from" << original_path << "to" << new_path << "has failed.";
                return false;
            }

            FS::updateTimestamp(new_path);

            QFileInfo new_path_file_info(new_path);
            resource.setFile(new_path_file_info);

            if (!m_is_watching)
                return update();

            return true;
        }
        case ResourceType::FOLDER: {
            if (QFile::exists(new_path)) {
                qDebug() << "Ignoring folder '" << original_path << "', it would merge with" << new_path;
                return false;
            }

            if (!FS::copy(original_path, new_path)()) {
                qWarning() << "Copy of folder from" << original_path << "to" << new_path << "has (potentially partially) failed.";
                return false;
            }

            QFileInfo newpathInfo(new_path);
            resource.setFile(newpathInfo);

            if (!m_is_watching)
                return update();

            return true;
        }
        default:
            break;
    }
    return false;
}

bool ResourceFolderModel::uninstallResource(QString file_name)
{
    for (auto& resource : m_resources) {
        if (resource->fileinfo().fileName() == file_name) {
            auto res = resource->destroy();

            update();

            return res;
        }
    }
    return false;
}

bool ResourceFolderModel::deleteResources(const QModelIndexList& indexes)
{
    if (!m_can_interact)
        return false;

    if (indexes.isEmpty())
        return true;

    for (auto i : indexes) {
        if (i.column() != 0) {
            continue;
        }

        auto& resource = m_resources.at(i.row());

        resource->destroy();
    }

    update();

    return true;
}

bool ResourceFolderModel::setResourceEnabled(const QModelIndexList &indexes, EnableAction action)
{
    if (!m_can_interact)
        return false;

    if (indexes.isEmpty())
        return true;

    bool succeeded = true;
    for (auto const& idx : indexes) {
        if (!validateIndex(idx) || idx.column() != 0)
            continue;

        int row = idx.row();

        auto& resource = m_resources[row];

        // Preserve the row, but change its ID
        auto old_id = resource->internal_id();
        if (!resource->enable(action)) {
            succeeded = false;
            continue;
        }

        auto new_id = resource->internal_id();
        if (m_resources_index.contains(new_id)) {
            // FIXME: https://github.com/PolyMC/PolyMC/issues/550
        }

        m_resources_index.remove(old_id);
        m_resources_index[new_id] = row;

        emit dataChanged(index(row, 0), index(row, columnCount(QModelIndex()) - 1));
    }

    return succeeded;
}

static QMutex s_update_task_mutex;
bool ResourceFolderModel::update()
{
    // We hold a lock here to prevent race conditions on the m_current_update_task reset.
    QMutexLocker lock(&s_update_task_mutex);

    // Already updating, so we schedule a future update and return.
    if (m_current_update_task) {
        m_scheduled_update = true;
        return false;
    }

    m_current_update_task.reset(createUpdateTask());
    if (!m_current_update_task)
        return false;

    connect(m_current_update_task.get(), &Task::succeeded, this, &ResourceFolderModel::onUpdateSucceeded,
            Qt::ConnectionType::QueuedConnection);
    connect(m_current_update_task.get(), &Task::failed, this, &ResourceFolderModel::onUpdateFailed, Qt::ConnectionType::QueuedConnection);
    connect(m_current_update_task.get(), &Task::finished, this, [=] {
        m_current_update_task.reset();
        if (m_scheduled_update) {
            m_scheduled_update = false;
            update();
        } else {
            emit updateFinished();
        }
    }, Qt::ConnectionType::QueuedConnection);

    QThreadPool::globalInstance()->start(m_current_update_task.get());

    return true;
}

void ResourceFolderModel::resolveResource(Resource* res)
{
    if (!res->shouldResolve()) {
        return;
    }

    Task::Ptr task{ createParseTask(*res) };
    if (!task)
        return;

    int ticket = m_next_resolution_ticket.fetch_add(1);

    res->setResolving(true, ticket);
    m_active_parse_tasks.insert(ticket, task);

    connect(
        task.get(), &Task::succeeded, this, [=] { onParseSucceeded(ticket, res->internal_id()); }, Qt::ConnectionType::QueuedConnection);
    connect(
        task.get(), &Task::failed, this, [=] { onParseFailed(ticket, res->internal_id()); }, Qt::ConnectionType::QueuedConnection);
    connect(
        task.get(), &Task::finished, this, [=] { m_active_parse_tasks.remove(ticket); }, Qt::ConnectionType::QueuedConnection);

    m_helper_thread_task.addTask(task);

    if (!m_helper_thread_task.isRunning()) {
        QThreadPool::globalInstance()->start(&m_helper_thread_task);
    }
}

void ResourceFolderModel::onUpdateSucceeded()
{
    auto update_results = static_cast<BasicFolderLoadTask*>(m_current_update_task.get())->result();

    auto& new_resources = update_results->resources;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    auto current_list = m_resources_index.keys();
    QSet<QString> current_set(current_list.begin(), current_list.end());

    auto new_list = new_resources.keys();
    QSet<QString> new_set(new_list.begin(), new_list.end());
#else
    QSet<QString> current_set(m_resources_index.keys().toSet());
    QSet<QString> new_set(new_resources.keys().toSet());
#endif

    applyUpdates(current_set, new_set, new_resources);
}

void ResourceFolderModel::onParseSucceeded(int ticket, QString resource_id)
{
    auto iter = m_active_parse_tasks.constFind(ticket);
    if (iter == m_active_parse_tasks.constEnd())
        return;

    int row = m_resources_index[resource_id];
    emit dataChanged(index(row), index(row, columnCount(QModelIndex()) - 1));
}

Task* ResourceFolderModel::createUpdateTask()
{
    return new BasicFolderLoadTask(m_dir);
}

bool ResourceFolderModel::hasPendingParseTasks() const
{
    return !m_active_parse_tasks.isEmpty();
}

void ResourceFolderModel::directoryChanged(QString path)
{
    update();
}

Qt::DropActions ResourceFolderModel::supportedDropActions() const
{
    // copy from outside, move from within and other resource lists
    return Qt::CopyAction | Qt::MoveAction;
}

Qt::ItemFlags ResourceFolderModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
    auto flags = defaultFlags;
    if (!m_can_interact) {
        flags &= ~Qt::ItemIsDropEnabled;
    } else {
        flags |= Qt::ItemIsDropEnabled;
        if (index.isValid()) {
            flags |= Qt::ItemIsUserCheckable;
        }
    }
    return flags;
}

QStringList ResourceFolderModel::mimeTypes() const
{
    QStringList types;
    types << "text/uri-list";
    return types;
}

bool ResourceFolderModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int, int, const QModelIndex&)
{
    if (action == Qt::IgnoreAction) {
        return true;
    }

    // check if the action is supported
    if (!data || !(action & supportedDropActions())) {
        return false;
    }

    // files dropped from outside?
    if (data->hasUrls()) {
        auto urls = data->urls();
        for (auto url : urls) {
            // only local files may be dropped...
            if (!url.isLocalFile()) {
                continue;
            }
            // TODO: implement not only copy, but also move
            // FIXME: handle errors here
            installResource(url.toLocalFile());
        }
        return true;
    }
    return false;
}

bool ResourceFolderModel::validateIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return false;

    int row = index.row();
    if (row < 0 || row >= m_resources.size())
        return false;

    return true;
}

QVariant ResourceFolderModel::data(const QModelIndex& index, int role) const
{
    if (!validateIndex(index))
        return {};

    int row = index.row();
    int column = index.column();

    switch (role) {
        case Qt::DisplayRole:
            switch (column) {
                case NAME_COLUMN:
                    return m_resources[row]->name();
                case DATE_COLUMN:
                    return m_resources[row]->dateTimeChanged();
                default:
                    return {};
            }
        case Qt::ToolTipRole:
            if (column == NAME_COLUMN) {
                if (at(row).isSymLinkUnder(instDirPath())) {
                    return m_resources[row]->internal_id() +
                        tr("\nWarning: This resource is symbolically linked from elsewhere. Editing it will also change the original."
                           "\nCanonical Path: %1")
                            .arg(at(row).fileinfo().canonicalFilePath());;
                }
                if (at(row).isMoreThanOneHardLink()) {
                    return m_resources[row]->internal_id() +
                        tr("\nWarning: This resource is hard linked elsewhere. Editing it will also change the original.");
                }
            }
            
            return m_resources[row]->internal_id();
        case Qt::DecorationRole: {
            if (column == NAME_COLUMN && (at(row).isSymLinkUnder(instDirPath()) || at(row).isMoreThanOneHardLink()))
                return APPLICATION->getThemedIcon("status-yellow");

            return {};
        }
        case Qt::CheckStateRole:
            switch (column) {
                case ACTIVE_COLUMN:
                    return m_resources[row]->enabled() ? Qt::Checked : Qt::Unchecked;
                default:
                    return {};
            }
        default:
            return {};
    }
}

bool ResourceFolderModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    int row = index.row();
    if (row < 0 || row >= rowCount(index.parent()) || !index.isValid())
        return false;

    if (role == Qt::CheckStateRole)
        return setResourceEnabled({ index }, EnableAction::TOGGLE);

    return false;
}

QVariant ResourceFolderModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            switch (section) {
                case NAME_COLUMN:
                    return tr("Name");
                case DATE_COLUMN:
                    return tr("Last modified");
                default:
                    return {};
            }
        case Qt::ToolTipRole: {
            switch (section) {
                case ACTIVE_COLUMN:
                    //: Here, resource is a generic term for external resources, like Mods, Resource Packs, Shader Packs, etc.
                    return tr("Is the resource enabled?");
                case NAME_COLUMN:
                    //: Here, resource is a generic term for external resources, like Mods, Resource Packs, Shader Packs, etc.
                    return tr("The name of the resource.");
                case DATE_COLUMN:
                    //: Here, resource is a generic term for external resources, like Mods, Resource Packs, Shader Packs, etc.
                    return tr("The date and time this resource was last changed (or added).");
                default:
                    return {};
            }
        }
        default:
            break;
    }

    return {};
}

QSortFilterProxyModel* ResourceFolderModel::createFilterProxyModel(QObject* parent)
{
    return new ProxyModel(parent);
}

SortType ResourceFolderModel::columnToSortKey(size_t column) const
{
    Q_ASSERT(m_column_sort_keys.size() == columnCount());
    return m_column_sort_keys.at(column);
}

void ResourceFolderModel::enableInteraction(bool enabled)
{
    if (m_can_interact == enabled)
        return;

    m_can_interact = enabled;
    if (size())
        emit dataChanged(index(0), index(size() - 1));
}

/* Standard Proxy Model for createFilterProxyModel */
[[nodiscard]] bool ResourceFolderModel::ProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    auto* model = qobject_cast<ResourceFolderModel*>(sourceModel());
    if (!model)
        return true;

    const auto& resource = model->at(source_row);

    return resource.applyFilter(filterRegularExpression());
}

[[nodiscard]] bool ResourceFolderModel::ProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
{
    auto* model = qobject_cast<ResourceFolderModel*>(sourceModel());
    if (!model || !source_left.isValid() || !source_right.isValid() || source_left.column() != source_right.column()) {
        return QSortFilterProxyModel::lessThan(source_left, source_right);
    }

    // we are now guaranteed to have two valid indexes in the same column... we love the provided invariants unconditionally and
    // proceed.

    auto column_sort_key = model->columnToSortKey(source_left.column());
    auto const& resource_left = model->at(source_left.row());
    auto const& resource_right = model->at(source_right.row());

    auto compare_result = resource_left.compare(resource_right, column_sort_key);
    if (compare_result.first == 0)
        return QSortFilterProxyModel::lessThan(source_left, source_right);

    if (compare_result.second || sortOrder() != Qt::DescendingOrder)
        return (compare_result.first < 0);
    return (compare_result.first > 0);
}

QString ResourceFolderModel::instDirPath() const {
    return QFileInfo(m_instance->instanceRoot()).absoluteFilePath();
}
