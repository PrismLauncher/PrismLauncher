// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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

#pragma once

#include <QAbstractListModel>

#include "modplatform/ModIndex.h"
#include "net/NetJob.h"
#include "ui/pages/modplatform/modrinth/ModrinthPage.h"

class ModPage;
class Version;

namespace Modrinth {

using LogoMap = QMap<QString, QIcon>;
using LogoCallback = std::function<void(QString)>;

class ModpackListModel : public QAbstractListModel {
    Q_OBJECT

   public:
    ModpackListModel(ModrinthPage* parent);
    ~ModpackListModel() override = default;

    inline auto rowCount(const QModelIndex& parent) const -> int override { return parent.isValid() ? 0 : m_modpacks.size(); };
    inline auto columnCount(const QModelIndex& parent) const -> int override { return parent.isValid() ? 0 : 1; };
    inline auto flags(const QModelIndex& index) const -> Qt::ItemFlags override { return QAbstractListModel::flags(index); };

    auto debugName() const -> QString;

    /* Retrieve information from the model at a given index with the given role */
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    inline void setActiveJob(NetJob::Ptr ptr) { m_jobPtr = ptr; }

    /* Ask the API for more information */
    void fetchMore(const QModelIndex& parent) override;
    void refresh();
    void searchWithTerm(const QString& term, int sort, std::shared_ptr<ModFilterWidget::Filter> filter, bool filterChanged);

    bool hasActiveSearchJob() const { return m_jobPtr && m_jobPtr->isRunning(); }
    Task::Ptr activeSearchJob() { return hasActiveSearchJob() ? m_jobPtr : nullptr; }

    void getLogo(const QString& logo, const QString& logoUrl, LogoCallback callback);

    inline auto canFetchMore(const QModelIndex& parent) const -> bool override
    {
        return parent.isValid() ? false : m_searchState == CanPossiblyFetchMore;
    };

   public slots:
    void searchRequestFinished(QList<ModPlatform::IndexedPack::Ptr>& doc_all);
    void searchRequestFailed(QString reason);
    void searchRequestForOneSucceeded(ModPlatform::IndexedPack::Ptr);

   protected slots:

    void logoFailed(QString logo);
    void logoLoaded(QString logo, QIcon out);

    void performPaginatedSearch();

   protected:
    void requestLogo(QString file, QString url);

    inline auto getMineVersions() const -> std::vector<Version>;

   protected:
    ModrinthPage* m_parent;

    QList<ModPlatform::IndexedPack::Ptr> m_modpacks;

    LogoMap m_logoMap;
    QMap<QString, LogoCallback> m_waitingCallbacks;
    QStringList m_failedLogos;
    QStringList m_loadingLogos;

    QString m_currentSearchTerm;
    QString m_currentSort;
    std::shared_ptr<ModFilterWidget::Filter> m_filter;
    int m_nextSearchOffset = 0;
    enum SearchState { None, CanPossiblyFetchMore, ResetRequested, Finished } m_searchState = None;

    Task::Ptr m_jobPtr;

    std::shared_ptr<QByteArray> m_allResponse = std::make_shared<QByteArray>();
    QByteArray m_specific_response;

    int m_modpacks_per_page = 20;
};
}  // namespace Modrinth
