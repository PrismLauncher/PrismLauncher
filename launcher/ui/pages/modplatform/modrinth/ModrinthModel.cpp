// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "ModrinthModel.h"

#include "Application.h"
#include "BuildConfig.h"
#include "Json.h"
#include "modplatform/ModIndex.h"
#include "modplatform/modrinth/ModrinthAPI.h"
#include "net/NetJob.h"
#include "ui/widgets/ProjectItem.h"

#include "net/ApiDownload.h"

#include <QMessageBox>
#include <memory>

namespace Modrinth {

ModpackListModel::ModpackListModel(ModrinthPage* parent) : QAbstractListModel(parent), m_parent(parent) {}

auto ModpackListModel::debugName() const -> QString
{
    return m_parent->debugName();
}

/******** Make data requests ********/

void ModpackListModel::fetchMore(const QModelIndex& parent)
{
    if (parent.isValid())
        return;
    if (m_nextSearchOffset == 0) {
        qWarning() << "fetchMore with 0 offset is wrong...";
        return;
    }
    performPaginatedSearch();
}

auto ModpackListModel::data(const QModelIndex& index, int role) const -> QVariant
{
    int pos = index.row();
    if (pos >= m_modpacks.size() || pos < 0 || !index.isValid()) {
        return QString("INVALID INDEX %1").arg(pos);
    }

    auto pack = m_modpacks.at(pos);
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
            if (m_logoMap.contains(pack->logoName))
                return m_logoMap.value(pack->logoName);

            QIcon icon = QIcon::fromTheme("screenshot-placeholder");
            ((ModpackListModel*)this)->requestLogo(pack->logoName, pack->logoUrl);
            return icon;
        }
        case Qt::UserRole: {
            QVariant v;
            v.setValue(pack);
            return v;
        }
        case Qt::SizeHintRole:
            return QSize(0, 58);
        // Custom data
        case UserDataTypes::TITLE:
            return pack->name;
        case UserDataTypes::DESCRIPTION:
            return pack->description;
        case UserDataTypes::INSTALLED:
            return false;
        default:
            break;
    }

    return {};
}

bool ModpackListModel::setData(const QModelIndex& index, const QVariant& value, [[maybe_unused]] int role)
{
    int pos = index.row();
    if (pos >= m_modpacks.size() || pos < 0 || !index.isValid())
        return false;

    m_modpacks[pos] = value.value<ModPlatform::IndexedPack::Ptr>();

    return true;
}

void ModpackListModel::performPaginatedSearch()
{
    if (hasActiveSearchJob())
        return;
    static const ModrinthAPI api;

    if (m_currentSearchTerm.startsWith("#")) {
        auto projectId = m_currentSearchTerm.mid(1);
        if (!projectId.isEmpty()) {
            ResourceAPI::Callback<ModPlatform::IndexedPack::Ptr> callbacks;

            callbacks.on_fail = [this](QString reason, int) { searchRequestFailed(reason); };
            callbacks.on_succeed = [this](auto& pack) { searchRequestForOneSucceeded(pack); };
            callbacks.on_abort = [this] {
                qCritical() << "Search task aborted by an unknown reason!";
                searchRequestFailed("Aborted");
            };
            auto project = std::make_shared<ModPlatform::IndexedPack>();
            project->addonId = projectId;
            if (auto job = api.getProjectInfo({ project }, std::move(callbacks)); job) {
                m_jobPtr = job;
                m_jobPtr->start();
            }
            return;
        }
    }  // TODO: Move to standalone API
    ResourceAPI::SortingMethod sort{};
    sort.name = m_currentSort;

    ResourceAPI::Callback<QList<ModPlatform::IndexedPack::Ptr>> callbacks{};

    callbacks.on_succeed = [this](auto& doc) { searchRequestFinished(doc); };
    callbacks.on_fail = [this](QString reason, int) { searchRequestFailed(reason); };
    callbacks.on_abort = [this] {
        qCritical() << "Search task aborted by an unknown reason!";
        searchRequestFailed("Aborted");
    };

    auto netJob = api.searchProjects({ ModPlatform::ResourceType::Modpack, m_nextSearchOffset, m_currentSearchTerm, sort, m_filter->loaders,
                                       m_filter->versions, ModPlatform::Side::NoSide, m_filter->categoryIds, m_filter->openSource },
                                     std::move(callbacks));

    m_jobPtr = netJob;
    m_jobPtr->start();
}

void ModpackListModel::refresh()
{
    if (hasActiveSearchJob()) {
        m_jobPtr->abort();
        m_searchState = ResetRequested;
        return;
    }

    beginResetModel();
    m_modpacks.clear();
    endResetModel();
    m_searchState = None;

    m_nextSearchOffset = 0;
    performPaginatedSearch();
}

static auto sortFromIndex(int index) -> QString
{
    switch (index) {
        default:
        case 0:
            return "relevance";
        case 1:
            return "downloads";
        case 2:
            return "follows";
        case 3:
            return "newest";
        case 4:
            return "updated";
    }
}

void ModpackListModel::searchWithTerm(const QString& term,
                                      const int sort,
                                      std::shared_ptr<ModFilterWidget::Filter> filter,
                                      bool filterChanged)
{
    if (sort > 5 || sort < 0)
        return;

    auto sort_str = sortFromIndex(sort);

    if (m_currentSearchTerm == term && m_currentSearchTerm.isNull() == term.isNull() && m_currentSort == sort_str && !filterChanged) {
        return;
    }

    m_currentSearchTerm = term;
    m_currentSort = sort_str;
    m_filter = filter;

    refresh();
}

void ModpackListModel::getLogo(const QString& logo, const QString& logoUrl, LogoCallback callback)
{
    if (m_logoMap.contains(logo)) {
        callback(APPLICATION->metacache()->resolveEntry(m_parent->metaEntryBase(), QString("logos/%1").arg(logo))->getFullPath());
    } else {
        requestLogo(logo, logoUrl);
    }
}

void ModpackListModel::requestLogo(QString logo, QString url)
{
    if (m_loadingLogos.contains(logo) || m_failedLogos.contains(logo) || url.isEmpty()) {
        return;
    }

    MetaEntryPtr entry = APPLICATION->metacache()->resolveEntry(m_parent->metaEntryBase(), QString("logos/%1").arg(logo));
    auto job = new NetJob(QString("%1 Icon Download %2").arg(m_parent->debugName()).arg(logo), APPLICATION->network());
    job->setAskRetry(false);
    job->addNetAction(Net::ApiDownload::makeCached(QUrl(url), entry));

    auto fullPath = entry->getFullPath();
    connect(job, &NetJob::succeeded, this, [this, logo, fullPath, job] {
        job->deleteLater();
        emit logoLoaded(logo, QIcon(fullPath));
        if (m_waitingCallbacks.contains(logo)) {
            m_waitingCallbacks.value(logo)(fullPath);
        }
    });

    connect(job, &NetJob::failed, this, [this, logo, job] {
        job->deleteLater();
        emit logoFailed(logo);
    });

    job->start();
    m_loadingLogos.append(logo);
}

/******** Request callbacks ********/

void ModpackListModel::logoLoaded(QString logo, QIcon out)
{
    m_loadingLogos.removeAll(logo);
    m_logoMap.insert(logo, out);
    for (int i = 0; i < m_modpacks.size(); i++) {
        if (m_modpacks[i]->logoName == logo) {
            emit dataChanged(createIndex(i, 0), createIndex(i, 0), { Qt::DecorationRole });
        }
    }
}

void ModpackListModel::logoFailed(QString logo)
{
    m_failedLogos.append(logo);
    m_loadingLogos.removeAll(logo);
}

void ModpackListModel::searchRequestFinished(QList<ModPlatform::IndexedPack::Ptr>& newList)
{
    m_jobPtr.reset();

    if (newList.size() < m_modpacks_per_page) {
        m_searchState = Finished;
    } else {
        m_nextSearchOffset += m_modpacks_per_page;
        m_searchState = CanPossiblyFetchMore;
    }

    // When you have a Qt build with assertions turned on, proceeding here will abort the application
    if (newList.size() == 0)
        return;

    beginInsertRows(QModelIndex(), m_modpacks.size(), m_modpacks.size() + newList.size() - 1);
    m_modpacks.append(newList);
    endInsertRows();
}

void ModpackListModel::searchRequestForOneSucceeded(ModPlatform::IndexedPack::Ptr pack)
{
    m_jobPtr.reset();

    beginInsertRows(QModelIndex(), m_modpacks.size(), m_modpacks.size() + 1);
    m_modpacks.append(pack);
    endInsertRows();
}

void ModpackListModel::searchRequestFailed(QString)
{
    auto failed_action = dynamic_cast<NetJob*>(m_jobPtr.get())->getFailedActions().at(0);
    if (failed_action->replyStatusCode() == -1) {
        // Network error
        QMessageBox::critical(nullptr, tr("Error"), tr("A network error occurred. Could not load modpacks."));
    } else if (failed_action->replyStatusCode() == 409) {
        // 409 Gone, notify user to update
        QMessageBox::critical(nullptr, tr("Error"),
                              //: %1 refers to the launcher itself
                              QString("%1 %2")
                                  .arg(m_parent->displayName())
                                  .arg(tr("API version too old!\nPlease update %1!").arg(BuildConfig.LAUNCHER_DISPLAYNAME)));
    }
    m_jobPtr.reset();

    if (m_searchState == ResetRequested) {
        beginResetModel();
        m_modpacks.clear();
        endResetModel();

        m_nextSearchOffset = 0;
        performPaginatedSearch();
    } else {
        m_searchState = Finished;
    }
}

}  // namespace Modrinth

/******** Helpers ********/
