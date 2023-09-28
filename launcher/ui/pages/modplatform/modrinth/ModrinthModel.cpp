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

#include "BuildConfig.h"
#include "Json.h"
#include "modplatform/modrinth/ModrinthAPI.h"
#include "net/NetJob.h"
#include "ui/widgets/ProjectItem.h"

#include "net/ApiDownload.h"

#include <QMessageBox>

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
    if (nextSearchOffset == 0) {
        qWarning() << "fetchMore with 0 offset is wrong...";
        return;
    }
    performPaginatedSearch();
}

auto ModpackListModel::data(const QModelIndex& index, int role) const -> QVariant
{
    int pos = index.row();
    if (pos >= modpacks.size() || pos < 0 || !index.isValid()) {
        return QString("INVALID INDEX %1").arg(pos);
    }

    Modrinth::Modpack pack = modpacks.at(pos);
    switch (role) {
        case Qt::ToolTipRole: {
            if (pack.description.length() > 100) {
                // some magic to prevent to long tooltips and replace html linebreaks
                QString edit = pack.description.left(97);
                edit = edit.left(edit.lastIndexOf("<br>")).left(edit.lastIndexOf(" ")).append("...");
                return edit;
            }
            return pack.description;
        }
        case Qt::DecorationRole: {
            if (m_logoMap.contains(pack.iconName))
                return m_logoMap.value(pack.iconName);

            QIcon icon = APPLICATION->getThemedIcon("screenshot-placeholder");
            ((ModpackListModel*)this)->requestLogo(pack.iconName, pack.iconUrl.toString());
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
            return pack.name;
        case UserDataTypes::DESCRIPTION:
            return pack.description;
        case UserDataTypes::SELECTED:
            return false;
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
    if (pos >= modpacks.size() || pos < 0 || !index.isValid())
        return false;

    modpacks[pos] = value.value<Modrinth::Modpack>();

    return true;
}

void ModpackListModel::performPaginatedSearch()
{
    if (hasActiveSearchJob())
        return;

    if (currentSearchTerm.startsWith("#")) {
        auto projectId = currentSearchTerm.mid(1);
        if (!projectId.isEmpty()) {
            ResourceAPI::ProjectInfoCallbacks callbacks;

            callbacks.on_fail = [this](QString reason) { searchRequestFailed(reason); };
            callbacks.on_succeed = [this](auto& doc, auto& pack) { searchRequestForOneSucceeded(doc); };
            static const ModrinthAPI api;
            if (auto job = api.getProjectInfo({ projectId }, std::move(callbacks)); job) {
                jobPtr = job;
                jobPtr->start();
            }
            return;
        }
    }  // TODO: Move to standalone API
    auto netJob = makeShared<NetJob>("Modrinth::SearchModpack", APPLICATION->network());
    auto searchAllUrl = QString(BuildConfig.MODRINTH_PROD_URL +
                                "/search?"
                                "offset=%1&"
                                "limit=%2&"
                                "query=%3&"
                                "index=%4&"
                                "facets=[[\"project_type:modpack\"]]")
                            .arg(nextSearchOffset)
                            .arg(m_modpacks_per_page)
                            .arg(currentSearchTerm)
                            .arg(currentSort);

    netJob->addNetAction(Net::ApiDownload::makeByteArray(QUrl(searchAllUrl), m_all_response));

    QObject::connect(netJob.get(), &NetJob::succeeded, this, [this] {
        QJsonParseError parse_error_all{};

        QJsonDocument doc_all = QJsonDocument::fromJson(*m_all_response, &parse_error_all);
        if (parse_error_all.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from " << debugName() << " at " << parse_error_all.offset
                       << " reason: " << parse_error_all.errorString();
            qWarning() << *m_all_response;
            return;
        }

        searchRequestFinished(doc_all);
    });
    QObject::connect(netJob.get(), &NetJob::failed, this, &ModpackListModel::searchRequestFailed);

    jobPtr = netJob;
    jobPtr->start();
}

void ModpackListModel::refresh()
{
    if (hasActiveSearchJob()) {
        jobPtr->abort();
        searchState = ResetRequested;
        return;
    }

    beginResetModel();
    modpacks.clear();
    endResetModel();
    searchState = None;

    nextSearchOffset = 0;
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

void ModpackListModel::searchWithTerm(const QString& term, const int sort)
{
    if (sort > 5 || sort < 0)
        return;

    auto sort_str = sortFromIndex(sort);

    if (currentSearchTerm == term && currentSearchTerm.isNull() == term.isNull() && currentSort == sort_str) {
        return;
    }

    currentSearchTerm = term;
    currentSort = sort_str;

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
    job->addNetAction(Net::ApiDownload::makeCached(QUrl(url), entry));

    auto fullPath = entry->getFullPath();
    QObject::connect(job, &NetJob::succeeded, this, [this, logo, fullPath, job] {
        job->deleteLater();
        emit logoLoaded(logo, QIcon(fullPath));
        if (waitingCallbacks.contains(logo)) {
            waitingCallbacks.value(logo)(fullPath);
        }
    });

    QObject::connect(job, &NetJob::failed, this, [this, logo, job] {
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
    for (int i = 0; i < modpacks.size(); i++) {
        if (modpacks[i].iconName == logo) {
            emit dataChanged(createIndex(i, 0), createIndex(i, 0), { Qt::DecorationRole });
        }
    }
}

void ModpackListModel::logoFailed(QString logo)
{
    m_failedLogos.append(logo);
    m_loadingLogos.removeAll(logo);
}

void ModpackListModel::searchRequestFinished(QJsonDocument& doc_all)
{
    jobPtr.reset();

    QList<Modrinth::Modpack> newList;

    auto packs_all = doc_all.object().value("hits").toArray();
    for (auto packRaw : packs_all) {
        auto packObj = packRaw.toObject();

        Modrinth::Modpack pack;
        try {
            Modrinth::loadIndexedPack(pack, packObj);
            newList.append(pack);
        } catch (const JSONValidationError& e) {
            qWarning() << "Error while loading mod from " << m_parent->debugName() << ": " << e.cause();
            continue;
        }
    }

    if (packs_all.size() < m_modpacks_per_page) {
        searchState = Finished;
    } else {
        nextSearchOffset += m_modpacks_per_page;
        searchState = CanPossiblyFetchMore;
    }

    // When you have a Qt build with assertions turned on, proceeding here will abort the application
    if (newList.size() == 0)
        return;

    beginInsertRows(QModelIndex(), modpacks.size(), modpacks.size() + newList.size() - 1);
    modpacks.append(newList);
    endInsertRows();
}

void ModpackListModel::searchRequestForOneSucceeded(QJsonDocument& doc)
{
    jobPtr.reset();

    auto packObj = doc.object();

    Modrinth::Modpack pack;
    try {
        Modrinth::loadIndexedPack(pack, packObj);
        pack.id = Json::ensureString(packObj, "id", pack.id);
    } catch (const JSONValidationError& e) {
        qWarning() << "Error while loading mod from " << m_parent->debugName() << ": " << e.cause();
        return;
    }

    beginInsertRows(QModelIndex(), modpacks.size(), modpacks.size() + 1);
    modpacks.append({ pack });
    endInsertRows();
}

void ModpackListModel::searchRequestFailed(QString reason)
{
    auto failed_action = dynamic_cast<NetJob*>(jobPtr.get())->getFailedActions().at(0);
    if (!failed_action->m_reply) {
        // Network error
        QMessageBox::critical(nullptr, tr("Error"), tr("A network error occurred. Could not load modpacks."));
    } else if (failed_action->m_reply && failed_action->m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 409) {
        // 409 Gone, notify user to update
        QMessageBox::critical(nullptr, tr("Error"),
                              //: %1 refers to the launcher itself
                              QString("%1 %2")
                                  .arg(m_parent->displayName())
                                  .arg(tr("API version too old!\nPlease update %1!").arg(BuildConfig.LAUNCHER_DISPLAYNAME)));
    }
    jobPtr.reset();

    if (searchState == ResetRequested) {
        beginResetModel();
        modpacks.clear();
        endResetModel();

        nextSearchOffset = 0;
        performPaginatedSearch();
    } else {
        searchState = Finished;
    }
}

}  // namespace Modrinth

/******** Helpers ********/
