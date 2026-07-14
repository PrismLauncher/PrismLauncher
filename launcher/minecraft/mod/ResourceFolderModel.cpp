#include "ResourceFolderModel.h"
#include <QMessageBox>

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QHeaderView>
#include <QIcon>
#include <QMenu>
#include <QMimeData>
#include <QStyle>
#include <QThreadPool>
#include <QUrl>
#include <algorithm>
#include <utility>

#include "Application.h"
#include "FileSystem.h"

#include "minecraft/mod/tasks/ResourceFolderLoadTask.h"

#include "Json.h"
#include "minecraft/mod/tasks/LocalResourceUpdateTask.h"
#include "modplatform/flame/FlameAPI.h"
#include "modplatform/flame/FlameModIndex.h"
#include "settings/Setting.h"
#include "tasks/SequentialTask.h"
#include "tasks/Task.h"
#include "ui/dialogs/CustomMessageBox.h"

ResourceFolderModel::ResourceFolderModel(const QDir& dir, BaseInstance* instance, bool isIndexed, bool createDir, QObject* parent)
    : QAbstractListModel(parent), m_dir(dir), m_instance(instance), m_watcher(this), m_isIndexed(isIndexed)
{
    if (createDir) {
        FS::ensureFolderPathExists(m_dir.absolutePath());
    }

    m_dir.setFilter(QDir::Readable | QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
    m_dir.setSorting(QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &ResourceFolderModel::directoryChanged);
    connect(&m_resourceResolver, &ConcurrentTask::finished, this, [this] {
        m_resourceResolver.clear();
        m_resourceResolverRunning = false;
    });
    if (APPLICATION_DYN) {  // in tests the application macro doesn't work
        m_resourceResolver.setMaxConcurrent(APPLICATION->settings()->get("NumberOfConcurrentTasks").toInt());
    }
}

ResourceFolderModel::~ResourceFolderModel()
{
    while (!QThreadPool::globalInstance()->waitForDone(100)) {
        QCoreApplication::processEvents();
    }
}

bool ResourceFolderModel::startWatching(const QStringList& paths)
{
    // Remove orphaned metadata next time
    m_firstFolderLoad = true;

    if (m_isWatching) {
        return false;
    }

    auto couldntBeWatched = m_watcher.addPaths(paths);
    for (const auto& path : paths) {
        if (couldntBeWatched.contains(path)) {
            qDebug() << "Failed to start watching" << path;
        } else {
            qDebug() << "Started watching" << path;
        }
    }

    update();

    m_isWatching = !m_isWatching;
    return m_isWatching;
}

bool ResourceFolderModel::stopWatching(const QStringList& paths)
{
    if (!m_isWatching) {
        return false;
    }

    auto couldntBeStopped = m_watcher.removePaths(paths);
    for (const auto& path : paths) {
        if (couldntBeStopped.contains(path)) {
            qDebug() << "Failed to stop watching" << path;
        } else {
            qDebug() << "Stopped watching" << path;
        }
    }

    m_isWatching = !m_isWatching;
    return !m_isWatching;
}

bool ResourceFolderModel::installResource(QString originalPath)
{
    // NOTE: fix for GH-1178: remove trailing slash to avoid issues with using the empty result of QFileInfo::fileName
    originalPath = FS::NormalizePath(originalPath);
    QFileInfo fileInfo(originalPath);

    if (!fileInfo.exists() || !fileInfo.isReadable()) {
        qWarning() << "Caught attempt to install non-existing file or file-like object:" << originalPath;
        return false;
    }
    qDebug() << "Installing:" << fileInfo.absoluteFilePath();

    Resource resource(fileInfo);
    if (!resource.valid()) {
        qWarning() << originalPath << "is not a valid resource. Ignoring it.";
        return false;
    }

    auto newPath = FS::NormalizePath(m_dir.filePath(fileInfo.fileName()));
    if (originalPath == newPath) {
        qWarning() << "Overwriting the mod (" << originalPath << ") with itself makes no sense...";
        return false;
    }

    switch (resource.type()) {
        case ResourceType::SINGLEFILE:
        case ResourceType::ZIPFILE:
        case ResourceType::LITEMOD: {
            if (QFile::exists(newPath) || QFile::exists(newPath + QString(".disabled"))) {
                if (!FS::deletePath(newPath)) {
                    qCritical() << "Cleaning up new location (" << newPath << ") was unsuccessful!";
                    return false;
                }
                qDebug() << newPath << "has been deleted.";
            }

            if (!QFile::copy(originalPath, newPath)) {
                qCritical() << "Copy from" << originalPath << "to" << newPath << "has failed.";
                return false;
            }

            FS::updateTimestamp(newPath);

            QFileInfo newPathFileInfo(newPath);
            resource.setFile(newPathFileInfo);

            if (!m_isWatching) {
                return update();
            }

            return true;
        }
        case ResourceType::FOLDER: {
            if (QFile::exists(newPath)) {
                qDebug() << "Ignoring folder '" << originalPath << "', it would merge with" << newPath;
                return false;
            }

            if (!FS::copy(originalPath, newPath)()) {
                qWarning() << "Copy of folder from" << originalPath << "to" << newPath << "has (potentially partially) failed.";
                return false;
            }

            QFileInfo newpathInfo(newPath);
            resource.setFile(newpathInfo);

            if (!m_isWatching) {
                return update();
            }

            return true;
        }
        default:
            break;
    }
    return false;
}

void ResourceFolderModel::installResourceWithFlameMetadata(const QString& path, ModPlatform::IndexedVersion& vers)
{
    auto install = [this, path] { installResource(path); };
    if (vers.addonId.isValid()) {
        ModPlatform::IndexedPack pack{
            .addonId = vers.addonId,
            .provider = ModPlatform::ResourceProvider::FLAME,
        };

        auto [job, response] = FlameAPI().getProject(vers.addonId.toString());
        connect(job.get(), &Task::failed, this, install);
        connect(job.get(), &Task::aborted, this, install);
        connect(job.get(), &Task::succeeded, [response, this, &vers, install, &pack] {
            QJsonParseError parseError{};
            QJsonDocument doc = QJsonDocument::fromJson(*response, &parseError);
            if (parseError.error != QJsonParseError::NoError) {
                qWarning() << "Error while parsing JSON response for mod info at" << parseError.offset
                           << "reason:" << parseError.errorString();
                qDebug() << *response;
                return;
            }
            try {
                auto obj = Json::requireObject(Json::requireObject(doc), "data");
                FlameMod::loadIndexedPack(pack, obj);
            } catch (const JSONValidationError& e) {
                qDebug() << doc;
                qWarning() << "Error while reading mod info:" << e.cause();
            }
            LocalResourceUpdateTask updateMetadata(indexDir(), pack, vers);
            connect(&updateMetadata, &Task::finished, this, install);
            updateMetadata.start();
        });

        job->start();
    } else {
        install();
    }
}

bool ResourceFolderModel::uninstallResource(const QString& fileName, bool preserveMetadata)
{
    for (auto& resource : m_resources) {
        auto resourceFileInfo = resource->fileinfo();
        auto resourceFileName = resource->fileinfo().fileName();
        if (!resource->enabled() && resourceFileName.endsWith(".disabled")) {
            resourceFileName.chop(9);
        }

        if (resourceFileName == fileName) {
            auto res = resource->destroy(indexDir(), preserveMetadata, false);

            update();

            return res;
        }
    }
    return false;
}

bool ResourceFolderModel::deleteResources(const QModelIndexList& indexes)
{
    if (indexes.isEmpty()) {
        return true;
    }

    for (auto i : indexes) {
        if (i.column() != 0) {
            continue;
        }

        const auto& resource = m_resources.at(i.row());
        resource->destroy(indexDir());
    }

    update();

    return true;
}

void ResourceFolderModel::deleteMetadata(const QModelIndexList& indexes)
{
    if (indexes.isEmpty()) {
        return;
    }

    for (auto i : indexes) {
        if (i.column() != 0) {
            continue;
        }

        const auto& resource = m_resources.at(i.row());
        resource->destroyMetadata(indexDir());
    }

    update();
}

bool ResourceFolderModel::setResourceEnabled(const QModelIndexList& indexes, EnableAction action)
{
    if (m_instance != nullptr && m_instance->isRunning()) {
        auto response =
            CustomMessageBox::selectable(nullptr, tr("Confirm toggle"),
                                         tr("If you enable/disable this resource while the game is running it may crash your game.\n"
                                            "Are you sure you want to do this?"),
                                         QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                ->exec();

        if (response != QMessageBox::Yes) {
            return false;
        }
    }

    if (indexes.isEmpty()) {
        return true;
    }

    bool succeeded = true;
    for (const auto& idx : indexes) {
        if (!validateIndex(idx) || idx.column() != 0) {
            continue;
        }

        int row = idx.row();

        auto& resource = m_resources[row];

        // Preserve the row, but change its ID
        auto oldId = resource->internalId();
        if (!resource->enable(action)) {
            succeeded = false;
            continue;
        }

        auto newId = resource->internalId();

        m_resourcesIndex.remove(oldId);
        m_resourcesIndex[newId] = row;

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
    if (m_currentUpdateTask) {
        m_scheduledUpdate = true;
        return false;
    }

    m_currentUpdateTask.reset(createUpdateTask());
    if (!m_currentUpdateTask) {
        return false;
    }

    connect(m_currentUpdateTask.get(), &Task::succeeded, this, &ResourceFolderModel::onUpdateSucceeded,
            Qt::ConnectionType::QueuedConnection);
    connect(m_currentUpdateTask.get(), &Task::failed, this, &ResourceFolderModel::onUpdateFailed, Qt::ConnectionType::QueuedConnection);
    connect(
        m_currentUpdateTask.get(), &Task::finished, this,
        [this] {
            m_currentUpdateTask.reset();
            if (m_scheduledUpdate) {
                m_scheduledUpdate = false;
                update();
            } else {
                emit updateFinished();
            }
        },
        Qt::ConnectionType::QueuedConnection);

    Task::Ptr preUpdate{ createPreUpdateTask() };

    if (preUpdate != nullptr) {
        auto* task = new SequentialTask("ResourceFolderModel::update");

        task->addTask(preUpdate);
        task->addTask(m_currentUpdateTask);

        connect(task, &Task::finished, [task] { task->deleteLater(); });

        QThreadPool::globalInstance()->start(task);
    } else {
        QThreadPool::globalInstance()->start(m_currentUpdateTask.get());
    }

    return true;
}

void ResourceFolderModel::resolveResource(Resource::Ptr res)
{
    if (!res->shouldResolve()) {
        return;
    }

    Task::Ptr task{ createParseTask(*res) };
    if (!task) {
        return;
    }

    int ticket = m_nextResolutionTicket.fetch_add(1);

    res->setResolving(true, ticket);
    m_activeParseTasks.insert(ticket, task);

    connect(
        task.get(), &Task::succeeded, this, [this, ticket, res] { onParseSucceeded(ticket, res->internalId()); },
        Qt::ConnectionType::QueuedConnection);
    connect(
        task.get(), &Task::failed, this, [this, ticket, res] { onParseFailed(ticket, res->internalId()); },
        Qt::ConnectionType::QueuedConnection);
    connect(
        task.get(), &Task::finished, this,
        [this, ticket] {
            m_activeParseTasks.remove(ticket);
            emit parseFinished();
        },
        Qt::ConnectionType::QueuedConnection);

    m_resourceResolver.addTask(task);

    if (!m_resourceResolverRunning) {
        QThreadPool::globalInstance()->start(&m_resourceResolver);
        m_resourceResolverRunning = true;
    }
}

void ResourceFolderModel::onUpdateSucceeded()
{
    auto updateResults = static_cast<ResourceFolderLoadTask*>(m_currentUpdateTask.get())->result();

    auto& newResources = updateResults->resources;

    auto currentList = m_resourcesIndex.keys();
    QSet<QString> currentSet(currentList.begin(), currentList.end());

    auto newList = newResources.keys();
    QSet<QString> newSet(newList.begin(), newList.end());

    applyUpdates(currentSet, newSet, newResources);
}

void ResourceFolderModel::onParseSucceeded(int ticket, const QString& resourceId)
{
    auto iter = m_activeParseTasks.constFind(ticket);
    if (iter == m_activeParseTasks.constEnd() || !m_resourcesIndex.contains(resourceId)) {
        return;
    }

    int row = m_resourcesIndex[resourceId];
    emit dataChanged(index(row), index(row, columnCount(QModelIndex()) - 1));
}

Task* ResourceFolderModel::createUpdateTask()
{
    auto indexDir2 = indexDir();
    auto* task = new ResourceFolderLoadTask(dir(), indexDir2, m_isIndexed, m_firstFolderLoad,
                                            [this](const QFileInfo& file) { return createResource(file); });
    m_firstFolderLoad = false;
    return task;
}

bool ResourceFolderModel::hasPendingParseTasks() const
{
    return !m_activeParseTasks.isEmpty();
}

void ResourceFolderModel::directoryChanged(const QString& /*path*/)
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
    auto flags = defaultFlags | Qt::ItemIsDropEnabled;
    if (index.isValid()) {
        flags |= Qt::ItemIsUserCheckable;
    }
    return flags;
}

QStringList ResourceFolderModel::mimeTypes() const
{
    QStringList types;
    types << "text/uri-list";
    return types;
}

bool ResourceFolderModel::dropMimeData(const QMimeData* data,
                                       Qt::DropAction action,
                                       int /*row*/,
                                       int /*column*/,
                                       const QModelIndex& /*parent*/)
{
    if (action == Qt::IgnoreAction) {
        return true;
    }

    // check if the action is supported
    if ((data == nullptr) || !(action & supportedDropActions())) {
        return false;
    }

    // files dropped from outside?
    if (data->hasUrls()) {
        auto urls = data->urls();
        for (const auto& url : urls) {
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
    if (!index.isValid()) {
        return false;
    }

    int row = index.row();
    return row >= 0 && row < m_resources.size();
}

// HACK: all subclasses need to call this to have the whole row painted
// and they only delegate to the superclass for compatible columns
QBrush ResourceFolderModel::rowBackground(int row) const
{
    if (APPLICATION->settings()->get("ShowModIncompat").toBool() && m_resources[row]->hasIssues()) {
        return { QColor(255, 0, 0, 40) };
    }
    return {};
}

QVariant ResourceFolderModel::data(const QModelIndex& index, int role) const
{
    if (!validateIndex(index)) {
        return {};
    }

    int row = index.row();
    int column = index.column();

    switch (role) {
        case Qt::BackgroundRole:
            return rowBackground(row);
        case Qt::DisplayRole:
            switch (column) {
                case NameColumn:
                    return m_resources[row]->name();
                case DateColumn:
                    return m_resources[row]->dateTimeChanged();
                case ProviderColumn:
                    return m_resources[row]->provider();
                case SizeColumn:
                    return m_resources[row]->sizeStr();
                case FileNameColumn:
                    return m_resources[row]->fileinfo().fileName();
                default:
                    return {};
            }
        case Qt::ToolTipRole: {
            QString tooltip = m_resources[row]->internalId();

            if (column == NameColumn) {
                if (APPLICATION->settings()->get("ShowModIncompat").toBool()) {
                    for (const QString& issue : at(row).issues()) {
                        tooltip += "\n" + issue;
                    }
                }

                if (at(row).isSymLinkUnder(instDirPath())) {
                    tooltip +=
                        m_resources[row]->internalId() +
                        tr("\nWarning: This resource is symbolically linked from elsewhere. Editing it will also change the original."
                           "\nCanonical Path: %1")
                            .arg(at(row).fileinfo().canonicalFilePath());
                }

                if (at(row).isMoreThanOneHardLink()) {
                    tooltip += tr("\nWarning: This resource is hard linked elsewhere. Editing it will also change the original.");
                }
            }

            return tooltip;
        }
        case Qt::DecorationRole: {
            if (column == NameColumn) {
                if (APPLICATION->settings()->get("ShowModIncompat").toBool() && at(row).hasIssues()) {
                    return QIcon::fromTheme("status-bad");
                }
                if (at(row).isSymLinkUnder(instDirPath()) || at(row).isMoreThanOneHardLink()) {
                    return QIcon::fromTheme("status-yellow");
                }
            }

            return {};
        }
        case Qt::CheckStateRole:
            if (column == ActiveColumn) {
                return m_resources[row]->enabled() ? Qt::Checked : Qt::Unchecked;
            }
            return {};
        default:
            return {};
    }
}

bool ResourceFolderModel::setData(const QModelIndex& index, [[maybe_unused]] const QVariant& value, int role)
{
    int row = index.row();
    if (row < 0 || row >= rowCount(index.parent()) || !index.isValid()) {
        return false;
    }

    if (role == Qt::CheckStateRole) {
        return setResourceEnabled({ index }, EnableAction::TOGGLE);
    }

    return false;
}

QVariant ResourceFolderModel::headerData(int section, [[maybe_unused]] Qt::Orientation orientation, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            switch (section) {
                case ActiveColumn:
                case NameColumn:
                case DateColumn:
                case ProviderColumn:
                case SizeColumn:
                case FileNameColumn:
                    return columnNames().at(section);
                default:
                    return {};
            }
        case Qt::ToolTipRole: {
            //: Here, resource is a generic term for external resources, like Mods, Resource Packs, Shader Packs, etc.
            switch (section) {
                case ActiveColumn:
                    return tr("Is the resource enabled?");
                case NameColumn:
                    return tr("The name of the resource.");
                case DateColumn:
                    return tr("The date and time this resource was last changed (or added).");
                case ProviderColumn:
                    return tr("The source provider of the resource.");
                case SizeColumn:
                    return tr("The size of the resource.");
                case FileNameColumn:
                    return tr("The file name of the resource.");
                default:
                    return {};
            }
        }
        default:
            break;
    }

    return {};
}

void ResourceFolderModel::setupHeaderAction(QAction* act, int column)
{
    Q_ASSERT(act);

    act->setText(columnNames().at(column));
}

void ResourceFolderModel::saveColumns(QTreeView* tree)
{
    const auto stateSettingName = QString("UI/%1_Page/Columns").arg(id());
    const auto overrideSettingName = QString("UI/%1_Page/ColumnsOverride").arg(id());
    const auto visibilitySettingName = QString("UI/%1_Page/ColumnsVisibility").arg(id());

    auto stateSetting = m_instance->settings()->getSetting(stateSettingName);
    stateSetting->set(QString::fromUtf8(tree->header()->saveState().toBase64()));

    // neither passthrough nor override settings works for this usecase as I need to only set the global when the gate is false
    auto* settings = m_instance->settings();
    if (!settings->get(overrideSettingName).toBool()) {
        settings = APPLICATION->settings();
    }
    auto visibility = Json::toMap(settings->get(visibilitySettingName).toString());
    for (auto i = 0; i < m_columnNames.size(); ++i) {
        if (m_columnsHideable[i]) {
            auto name = m_columnNames[i];
            visibility[name] = !tree->isColumnHidden(i);
        }
    }
    settings->set(visibilitySettingName, Json::fromMap(visibility));
}

void ResourceFolderModel::loadColumns(QTreeView* tree)
{
    const auto stateSettingName = QString("UI/%1_Page/Columns").arg(id());
    const auto overrideSettingName = QString("UI/%1_Page/ColumnsOverride").arg(id());
    const auto visibilitySettingName = QString("UI/%1_Page/ColumnsVisibility").arg(id());

    auto stateSetting = m_instance->settings()->getOrRegisterSetting(stateSettingName, "");
    tree->header()->restoreState(QByteArray::fromBase64(stateSetting->get().toString().toUtf8()));

    auto setVisible = [this, tree](const QVariant& value) {
        auto visibility = Json::toMap(value.toString());
        for (auto i = 0; i < m_columnNames.size(); ++i) {
            if (m_columnsHideable[i]) {
                auto name = m_columnNames[i];
                tree->setColumnHidden(i, !visibility.value(name, false).toBool());
            }
        }
    };

    const auto defaultValue = Json::fromMap({
        { "Image", true },
        { "Version", true },
        { "Last Modified", true },
        { "Provider", true },
        { "Pack Format", true },
    });
    // neither passthrough nor override settings works for this usecase as I need to only set the global when the gate is false
    auto* settings = m_instance->settings();
    if (!settings->getOrRegisterSetting(overrideSettingName, false)->get().toBool()) {
        settings = APPLICATION->settings();
    }
    auto visibility = settings->getOrRegisterSetting(visibilitySettingName, defaultValue);
    setVisible(visibility->get());

    // allways connect the signal in case the setting is toggled on and off
    auto gSetting = APPLICATION->settings()->getOrRegisterSetting(visibilitySettingName, defaultValue);
    connect(gSetting.get(), &Setting::SettingChanged, tree, [this, setVisible, overrideSettingName](const Setting&, const QVariant& value) {
        if (!m_instance->settings()->get(overrideSettingName).toBool()) {
            setVisible(value);
        }
    });
}

QMenu* ResourceFolderModel::createHeaderContextMenu(QTreeView* tree)
{
    auto* menu = new QMenu(tree);

    {  // action to decide if the visibility is per instance or not
        auto* act = new QAction(tr("Override Columns Visibility"), menu);
        const auto overrideSettingName = QString("UI/%1_Page/ColumnsOverride").arg(id());

        act->setCheckable(true);
        act->setChecked(m_instance->settings()->getOrRegisterSetting(overrideSettingName, false)->get().toBool());

        connect(act, &QAction::toggled, tree, [this, tree, overrideSettingName](bool toggled) {
            m_instance->settings()->set(overrideSettingName, toggled);
            saveColumns(tree);
        });

        menu->addAction(act);
    }
    menu->addSeparator()->setText(tr("Show / Hide Columns"));

    for (int col = 0; col < columnCount(); ++col) {
        // Skip creating actions for columns that should not be hidden
        if (!m_columnsHideable.at(col)) {
            continue;
        }
        auto* act = new QAction(menu);
        setupHeaderAction(act, col);

        act->setCheckable(true);
        act->setChecked(!tree->isColumnHidden(col));

        connect(act, &QAction::toggled, tree, [this, col, tree](bool toggled) {
            tree->setColumnHidden(col, !toggled);
            for (int c = 0; c < columnCount(); ++c) {
                if (m_columnResizeModes.at(c) == QHeaderView::ResizeToContents) {
                    tree->resizeColumnToContents(c);
                }
            }
            saveColumns(tree);
        });

        menu->addAction(act);
    }

    return menu;
}

QSortFilterProxyModel* ResourceFolderModel::createFilterProxyModel(QObject* parent)
{
    return new ProxyModel(parent);
}

SortType ResourceFolderModel::columnToSortKey(size_t column) const
{
    Q_ASSERT(m_columnSortKeys.size() == columnCount());
    return m_columnSortKeys.at(column);
}

/* Standard Proxy Model for createFilterProxyModel */
bool ResourceFolderModel::ProxyModel::filterAcceptsRow(int sourceRow, [[maybe_unused]] const QModelIndex& sourceParent) const
{
    auto* model = qobject_cast<ResourceFolderModel*>(sourceModel());
    if (!model) {
        return true;
    }

    const auto& resource = model->at(sourceRow);

    return resource.applyFilter(filterRegularExpression());
}

bool ResourceFolderModel::ProxyModel::lessThan(const QModelIndex& sourceLeft, const QModelIndex& sourceRight) const
{
    auto* model = qobject_cast<ResourceFolderModel*>(sourceModel());
    if (!model || !sourceLeft.isValid() || !sourceRight.isValid() || sourceLeft.column() != sourceRight.column()) {
        return QSortFilterProxyModel::lessThan(sourceLeft, sourceRight);
    }

    // we are now guaranteed to have two valid indexes in the same column... we love the provided invariants unconditionally and
    // proceed.

    auto columnSortKey = model->columnToSortKey(sourceLeft.column());
    const auto& resourceLeft = model->at(sourceLeft.row());
    const auto& resourceRight = model->at(sourceRight.row());

    auto compareResult = resourceLeft.compare(resourceRight, columnSortKey);
    if (compareResult == 0) {
        return QSortFilterProxyModel::lessThan(sourceLeft, sourceRight);
    }

    return compareResult < 0;
}

QString ResourceFolderModel::instDirPath() const
{
    return QFileInfo(m_instance->instanceRoot()).absoluteFilePath();
}

void ResourceFolderModel::onParseFailed(int ticket, const QString& resourceId)
{
    auto iter = m_activeParseTasks.constFind(ticket);
    if (iter == m_activeParseTasks.constEnd() || !m_resourcesIndex.contains(resourceId)) {
        return;
    }

    auto removedIndex = m_resourcesIndex[resourceId];
    auto removedIt = m_resources.begin() + removedIndex;
    Q_ASSERT(removedIt != m_resources.end());

    beginRemoveRows(QModelIndex(), removedIndex, removedIndex);
    m_resources.erase(removedIt);

    // update index
    m_resourcesIndex.clear();
    int idx = 0;
    for (const auto& mod : qAsConst(m_resources)) {
        m_resourcesIndex[mod->internalId()] = idx;
        idx++;
    }
    endRemoveRows();
}

void ResourceFolderModel::applyUpdates(QSet<QString>& currentSet, QSet<QString>& newSet, QMap<QString, Resource::Ptr>& newResources)
{
    // see if the kept resources changed in some way
    {
        QSet<QString> keptSet = currentSet;
        keptSet.intersect(newSet);

        for (const auto& kept : keptSet) {
            auto rowIt = m_resourcesIndex.constFind(kept);
            Q_ASSERT(rowIt != m_resourcesIndex.constEnd());
            auto row = rowIt.value();

            auto& newResource = newResources[kept];
            const auto& currentResource = m_resources.at(row);

            if (newResource->dateTimeChanged() == currentResource->dateTimeChanged()) {
                // no significant change
                bool hadIssues = !currentResource->hasIssues();
                currentResource->updateIssues(m_instance);

                if (hadIssues != currentResource->hasIssues()) {
                    emit dataChanged(index(row, 0), index(row, columnCount({}) - 1));
                }
                continue;
            }

            // If the resource is resolving, but something about it changed, we don't want to
            // continue the resolving.
            if (currentResource->isResolving()) {
                auto ticket = currentResource->resolutionTicket();
                if (m_activeParseTasks.contains(ticket)) {
                    auto* task = (*m_activeParseTasks.find(ticket)).get();
                    task->abort();
                }
            }

            m_resources[row].reset(newResource);
            newResource->updateIssues(m_instance);

            resolveResource(m_resources.at(row));
            emit dataChanged(index(row, 0), index(row, columnCount(QModelIndex()) - 1));
        }
    }

    // remove resources no longer present
    {
        QSet<QString> removedSet = currentSet;
        removedSet.subtract(newSet);

        QList<int> removedRows;
        for (const auto& removed : removedSet) {
            removedRows.append(m_resourcesIndex[removed]);
        }

        std::ranges::sort(removedRows, std::greater());

        for (auto& removedIndex : removedRows) {
            auto removedIt = m_resources.begin() + removedIndex;

            Q_ASSERT(removedIt != m_resources.end());

            if ((*removedIt)->isResolving()) {
                auto ticket = (*removedIt)->resolutionTicket();
                if (m_activeParseTasks.contains(ticket)) {
                    auto* task = (*m_activeParseTasks.find(ticket)).get();
                    task->abort();
                }
            }

            beginRemoveRows(QModelIndex(), removedIndex, removedIndex);
            m_resources.erase(removedIt);
            endRemoveRows();
        }
    }

    // add new resources to the end
    {
        QSet<QString> addedSet = newSet;
        addedSet.subtract(currentSet);

        // When you have a Qt build with assertions turned on, proceeding here will abort the application
        if (addedSet.size() > 0) {
            beginInsertRows(QModelIndex(), static_cast<int>(m_resources.size()),
                            static_cast<int>(m_resources.size() + addedSet.size() - 1));

            for (const auto& added : addedSet) {
                auto res = newResources[added];
                res->updateIssues(m_instance);
                m_resources.append(res);
                resolveResource(m_resources.last());
            }

            endInsertRows();
        }
    }

    // update index
    {
        m_resourcesIndex.clear();
        int idx = 0;
        for (const auto& mod : qAsConst(m_resources)) {
            m_resourcesIndex[mod->internalId()] = idx;
            idx++;
        }
    }
}
Resource::Ptr ResourceFolderModel::find(QString id)
{
    auto iter =
        std::find_if(m_resources.constBegin(), m_resources.constEnd(), [&](const Resource::Ptr& r) { return r->internalId() == id; });
    if (iter == m_resources.constEnd()) {
        return nullptr;
    }
    return *iter;
}
QList<Resource*> ResourceFolderModel::allResources()
{
    QList<Resource*> result;
    result.reserve(m_resources.size());
    for (const Resource ::Ptr& resource : m_resources) {
        result.append((resource.get()));
    }
    return result;
}

QList<Resource*> ResourceFolderModel::selectedResources(const QModelIndexList& indexes)
{
    QList<Resource*> result;
    for (const QModelIndex& index : indexes) {
        if (index.column() != 0) {
            continue;
        }
        result.append(&at(index.row()));
    }
    return result;
}
