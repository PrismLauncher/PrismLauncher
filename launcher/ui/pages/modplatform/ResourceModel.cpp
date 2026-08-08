// SPDX-FileCopyrightText: 2023 flowln <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "ResourceModel.h"

#include <QCryptographicHash>
#include <QIcon>
#include <QList>
#include <QMessageBox>
#include <QPixmapCache>
#include <QUrl>
#include <algorithm>
#include <memory>
#include <utility>
#include <variant>

#include "Application.h"
#include "BuildConfig.h"
#include "settings/SettingsObject.h"

#include "modplatform/ResourceAPI.h"
#include "net/ApiRequest.h"
#include "net/NetJob.h"

#include "modplatform/ModIndex.h"

#include "tasks/Task.h"
#include "ui/widgets/ProjectItem.h"

namespace ResourceDownload {

QHash<ResourceModel*, bool> ResourceModel::s_runningModels;

ResourceModel::ResourceModel(const ResourceAPI* api) : m_api(api)
{
    s_runningModels.insert(this, true);
    if (APPLICATION_DYN) {
        m_currentInfoJob.setMaxConcurrent(APPLICATION->settings()->get("NumberOfConcurrentDownloads").toInt());
    }
}

ResourceModel::~ResourceModel()
{
    s_runningModels.find(this).value() = false;
}

auto ResourceModel::data(const QModelIndex& index, int role) const -> QVariant
{
    int pos = index.row();
    if (pos >= m_packs.size() || pos < 0 || !index.isValid()) {
        return QString("INVALID INDEX %1").arg(pos);
    }

    auto pack = m_packs.at(pos);
    switch (role) {
        case Qt::ToolTipRole: {
            if (pack->description.length() > 100) {
                // some magic to prevent to long tooltips and replace html linebreaks
                QString edit = pack->description.left(97);
                edit = edit.left(edit.lastIndexOf("<br>")).left(edit.lastIndexOf(" ")).append("...");
                return edit;
            }
            return pack->description;
        }
        case Qt::DecorationRole: {
            if (APPLICATION_DYN) {
                if (auto iconOrNone = const_cast<ResourceModel*>(this)->getIcon(index, pack->logoUrl); iconOrNone.has_value()) {
                    return iconOrNone.value();
                }

                return QIcon::fromTheme("screenshot-placeholder");
            }
            return {};
        }
        case Qt::SizeHintRole:
            return QSize(0, 58);
        case Qt::UserRole: {
            QVariant v;
            v.setValue(pack);
            return v;
        }
            // Custom data
        case UserDataTypes::TITLE:
            return pack->name;
        case UserDataTypes::DESCRIPTION:
            return pack->description;
        case Qt::CheckStateRole:
            return pack->isAnyVersionSelected() ? Qt::Checked : Qt::Unchecked;
        case UserDataTypes::INSTALLED:
            return this->isPackInstalled(pack);
        default:
            break;
    }

    return {};
}

QHash<int, QByteArray> ResourceModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[Qt::ToolTipRole] = "toolTip";
    roles[Qt::DecorationRole] = "decoration";
    roles[Qt::SizeHintRole] = "sizeHint";
    roles[Qt::UserRole] = "pack";
    roles[UserDataTypes::TITLE] = "title";
    roles[UserDataTypes::DESCRIPTION] = "description";
    roles[UserDataTypes::INSTALLED] = "installed";

    return roles;
}

bool ResourceModel::setData(const QModelIndex& index, const QVariant& value, [[maybe_unused]] int role)
{
    int pos = index.row();
    if (pos >= m_packs.size() || pos < 0 || !index.isValid()) {
        return false;
    }

    m_packs[pos] = value.value<ModPlatform::IndexedPack::Ptr>();
    emit dataChanged(index, index);

    return true;
}

QString ResourceModel::debugName() const
{
    return "ResourceDownload (Model)";
}

void ResourceModel::fetchMore(const QModelIndex& parent)
{
    if (parent.isValid() || m_searchState == SearchState::Finished) {
        return;
    }

    search();
}

void ResourceModel::search()
{
    if (hasActiveSearchJob()) {
        return;
    }

    if (m_searchState != SearchState::ResetRequested && m_searchTerm.startsWith("#")) {
        auto projectId = m_searchTerm.mid(1);
        if (!projectId.isEmpty()) {
            auto [job, result] = m_api->getProject(projectId, true).make();
            if (job) {
                auto weak = job.toWeakRef();
                connect(job.get(), &Task::succeeded, this, [this, result] {
                    if (!s_runningModels.constFind(this).value()) {
                        return;
                    }
                    searchRequestForOneSucceeded(*result);
                });
                connect(job.get(), &Task::failed, this, [this, weak](const QString& reason) {
                    if (!s_runningModels.constFind(this).value()) {
                        return;
                    }
                    if (auto job = weak.lock()) {
                        if (job->replyStatusCode() == 404) {
                            m_searchState = SearchState::ResetRequested;
                        }
                        searchRequestFailed(reason, job->replyStatusCode());
                    }
                });
                connect(job.get(), &Task::aborted, this, [this] {
                    if (!s_runningModels.constFind(this).value()) {
                        return;
                    }
                    searchRequestAborted();
                });
                runSearchJob(job);
            }
            return;
        }
    }
    auto args{ createSearchArguments() };

    if (auto [job, result] = m_api->searchProjects(args).make(); job) {
        auto weak = job.toWeakRef();
        connect(job.get(), &Task::failed, this, [this, weak](const QString& reason) {
            if (!s_runningModels.constFind(this).value()) {
                return;
            }
            if (auto job = weak.lock()) {
                searchRequestFailed(reason, job->replyStatusCode());
            }
        });
        connect(job.get(), &Task::aborted, this, [this] {
            if (!s_runningModels.constFind(this).value()) {
                return;
            }
            searchRequestAborted();
        });
        connect(job.get(), &Task::succeeded, this, [this, result] {
            if (!s_runningModels.constFind(this).value()) {
                return;
            }
            searchRequestSucceeded(*result);
        });
        runSearchJob(job);
    } else {
        searchRequestFailed("Failed to create search URL", -1);
    }
}

void ResourceModel::loadEntry(const QModelIndex& entry)
{
    const auto& pack = m_packs[entry.row()];

    if (!hasActiveInfoJob()) {
        m_currentInfoJob.clear();
    }

    if (!pack->versionsLoaded) {
        auto args{ createVersionsArguments(entry) };

        auto addonId = pack->addonId;
        auto [job, result] = m_api->getProjectVersions(args).make();
        if (job) {
            connect(job.get(), &Task::succeeded, this, [this, entry, addonId, result] {
                if (!s_runningModels.constFind(this).value()) {
                    return;
                }
                }
                versionRequestSucceeded(*result, addonId, entry);
            });
            connect(job.get(), &Task::failed, this, [](const QString& reason) {
                QMessageBox::critical(nullptr, tr("Error"),
                                      tr("A network error occurred. Could not load project versions: %1").arg(reason));
            });
            runInfoJob(job);
        }
    }

    if (!pack->extraDataLoaded) {
        auto addonId = createInfoArguments(entry);
        auto [job, result] = m_api->getProject(addonId, true).make();
        if (job) {
            connect(job.get(), &Task::succeeded, this, [this, entry, result] {
                if (!s_runningModels.constFind(this).value()) {
                    return;
                }
                infoRequestSucceeded(*result, entry);
            });
            connect(job.get(), &Task::failed, this, [this](const QString& reason) {
                if (!s_runningModels.constFind(this).value()) {
                    return;
                }
                QMessageBox::critical(nullptr, tr("Error"), tr("A network error occurred. Could not load project info: %1").arg(reason));
            });
            connect(job.get(), &Task::aborted, this, [this] {
                if (!s_runningModels.constFind(this).value()) {
                    return;
                }
                qCritical() << tr("The request was aborted for an unknown reason");
            });
            runInfoJob(job);
        }
    }
}

void ResourceModel::refresh()
{
    bool resetRequested = false;

    if (hasActiveInfoJob()) {
        m_currentInfoJob.abort();
        resetRequested = true;
    }

    if (hasActiveSearchJob()) {
        m_currentSearchJob->abort();
        resetRequested = true;
    }

    if (resetRequested) {
        m_searchState = SearchState::ResetRequested;
        return;
    }

    clearData();
    m_searchState = SearchState::None;

    m_nextSearchOffset = 0;
    search();
}

void ResourceModel::clearData()
{
    beginResetModel();
    m_packs.clear();
    endResetModel();
}

void ResourceModel::runSearchJob(const Task::Ptr& ptr)
{
    m_currentSearchJob.reset(ptr);  // clean up first
    m_currentSearchJob->start();
}
void ResourceModel::runInfoJob(Task::Ptr ptr)
{
    if (!m_currentInfoJob.isRunning()) {
        m_currentInfoJob.clear();
    }

    m_currentInfoJob.addTask(std::move(ptr));

    if (!m_currentInfoJob.isRunning()) {
        m_currentInfoJob.run();
    }
}

std::optional<ResourceAPI::SortingMethod> ResourceModel::getCurrentSortingMethodByIndex() const
{
    std::optional<ResourceAPI::SortingMethod> sort{};

    {  // Find sorting method by ID
        auto sortingMethods = getSortingMethods();
        auto method = std::find_if(sortingMethods.constBegin(), sortingMethods.constEnd(),
                                   [this](const auto& e) { return m_currentSortIndex == e.index; });
        if (method != sortingMethods.constEnd()) {
            sort = *method;
        }
    }

    return sort;
}

std::optional<QIcon> ResourceModel::getIcon(const QModelIndex& index, const QUrl& url)
{
    QPixmap pixmap;
    if (QPixmapCache::find(url.toString(), &pixmap)) {
        return { pixmap };
    }

    if (!m_currentIconJob) {
        m_currentIconJob.reset(new NetJob("IconJob", APPLICATION->network()));
        m_currentIconJob->setAskRetry(false);
    }

    if (m_currentlyRunningIconActions.contains(url)) {
        return {};
    }
    if (m_failedIconActions.contains(url)) {
        return {};
    }

    auto cacheEntry = APPLICATION->metacache()->resolveEntry(
        metaEntryBase(),
        QString("logos/%1").arg(QString(QCryptographicHash::hash(url.toEncoded(), QCryptographicHash::Algorithm::Sha1).toHex())));
    auto iconFetchAction = Net::ApiRequest::makeCached(url, cacheEntry);

    auto fullFilePath = cacheEntry->getFullPath();
    connect(iconFetchAction.get(), &Task::succeeded, this, [this, url, fullFilePath, index] {
        auto icon = QIcon(fullFilePath);
        QPixmapCache::insert(url.toString(), icon.pixmap(icon.actualSize({ 64, 64 })));

        m_currentlyRunningIconActions.remove(url);

        emit dataChanged(index, index, { Qt::DecorationRole });
    });
    connect(iconFetchAction.get(), &Task::failed, this, [this, url] {
        m_currentlyRunningIconActions.remove(url);
        m_failedIconActions.insert(url);
    });

    m_currentlyRunningIconActions.insert(url);

    m_currentIconJob->addNetAction(iconFetchAction);
    if (!m_currentIconJob->isRunning()) {
        QMetaObject::invokeMethod(m_currentIconJob.get(), &NetJob::start);
    }

    return {};
}

/* Default callbacks */

void ResourceModel::searchRequestSucceeded(QList<ModPlatform::IndexedPack::Ptr>& newList)
{
    QList<ModPlatform::IndexedPack::Ptr> filteredNewList;
    for (auto pack : newList) {
        ModPlatform::IndexedPack::Ptr p;
        if (auto sel = std::ranges::find_if(m_selected,
                                            [&pack](const DownloadTaskPtr& i) {
                                                const auto ipack = i->getPack();
                                                return ipack->provider == pack->provider && ipack->addonId == pack->addonId;
                                            });
            sel != m_selected.end()) {
            p = sel->get()->getPack();
        } else {
            p = pack;
        }
        if (checkFilters(p)) {
            filteredNewList << p;
        }
    }

    if (newList.size() < 25) {
        m_searchState = SearchState::Finished;
    } else {
        m_nextSearchOffset += 25;
        m_searchState = SearchState::CanFetchMore;
    }

    // When you have a Qt build with assertions turned on, proceeding here will abort the application
    if (filteredNewList.size() == 0) {
        return;
    }

    beginInsertRows(QModelIndex(), static_cast<int>(m_packs.size()),
                    static_cast<int>(m_packs.size() + filteredNewList.size() - 1));
    m_packs.append(filteredNewList);
    endInsertRows();
}

void ResourceModel::searchRequestForOneSucceeded(const ModPlatform::IndexedPack::Ptr& pack)
{
    m_searchState = SearchState::Finished;

    beginInsertRows(QModelIndex(), static_cast<int>(m_packs.size()), static_cast<int>(m_packs.size() + 1));
    m_packs.append(pack);
    endInsertRows();
}

void ResourceModel::searchRequestFailed([[maybe_unused]] const QString& reason, int networkErrorCode)
{
    switch (networkErrorCode) {
        default:
            // Network error
            QMessageBox::critical(nullptr, tr("Error"), tr("A network error occurred. Could not load mods."));
            break;
        case 404:
            // 404 Not Found, some APIs return this when nothing is found, no need to bother the user
            break;
        case 409:
            // 409 Gone, notify user to update
            QMessageBox::critical(nullptr, tr("Error"),
                                  QString("%1").arg(tr("API version too old!\nPlease update %1!").arg(BuildConfig.LAUNCHER_DISPLAYNAME)));
            break;
    }

    if (m_searchState == SearchState::ResetRequested) {
        clearData();

        m_nextSearchOffset = 0;
        search();
    } else {
        m_searchState = SearchState::Finished;
    }
}

void ResourceModel::searchRequestAborted()
{
    if (m_searchState != SearchState::ResetRequested) {
        qCritical() << "Search task in" << debugName() << "aborted by an unknown reason!";
    }

    // Retry fetching
    clearData();

    m_nextSearchOffset = 0;
    search();
}

void ResourceModel::versionRequestSucceeded(QVector<ModPlatform::IndexedVersion>& doc, const QVariant& pack, const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }

    auto currentPack = data(index, Qt::UserRole).value<ModPlatform::IndexedPack::Ptr>();

    // Check if the index is still valid for this resource or not
    if (!currentPack || pack != currentPack->addonId) {
        return;
    }

    currentPack->versions = doc;
    currentPack->versionsLoaded = true;

    // Cache info :^)
    QVariant newPack;
    newPack.setValue(currentPack);
    if (!setData(index, newPack, Qt::UserRole)) {
        qWarning() << "Failed to cache resource versions!";
        return;
    }

    emit versionListUpdated(index);
}

void ResourceModel::infoRequestSucceeded(ModPlatform::IndexedPack::Ptr pack, const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }

    auto currentPack = data(index, Qt::UserRole).value<ModPlatform::IndexedPack::Ptr>();

    // Check if the index is still valid for this resource or not
    if (!currentPack || pack->addonId != currentPack->addonId) {
        return;
    }

    // Cache info :^)
    QVariant newPack;
    newPack.setValue(pack);
    if (!setData(index, newPack, Qt::UserRole)) {
        qWarning() << "Failed to cache resource info!";
        return;
    }

    emit projectInfoUpdated(index);
}

void ResourceModel::addPack(ModPlatform::IndexedPack::Ptr pack,
                            ModPlatform::IndexedVersion& version,
                            ResourceFolderModel* packs,
                            bool isIndexed,
                            QString downloadReason,
                            QString dependentOn)
{
version.isCurrentlySelected = true;
    m_selected.append(
        makeShared<ResourceDownloadTask>(std::move(pack), version, packs, isIndexed, std::move(downloadReason), std::move(dependentOn)));
}

void ResourceModel::removePack(const QString& rem)
{
    auto pred = [&rem](const DownloadTaskPtr& i) { return rem == i->getName(); };
    m_selected.removeIf(pred);
    auto pack = std::ranges::find_if(m_packs, [&rem](const ModPlatform::IndexedPack::Ptr& i) { return rem == i->name; });
    if (pack == m_packs.end()) {  // ignore it if is not in the current search
        return;
    }
    if (!pack->get()->versionsLoaded) {
        return;
    }
    for (auto& ver : pack->get()->versions) {
        ver.isCurrentlySelected = false;
    }
}

bool ResourceModel::checkVersionFilters(const ModPlatform::IndexedVersion& v)
{
    return (!optedOut(v));
}
}  // namespace ResourceDownload
