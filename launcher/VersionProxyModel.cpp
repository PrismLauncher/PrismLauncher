// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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

#include "VersionProxyModel.h"
#include <Version.h>
#include <meta/VersionList.h>
#include <QIcon>
#include <QPixmapCache>
#include <QSortFilterProxyModel>

class VersionFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
   public:
    explicit VersionFilterModel(VersionProxyModel* parent) : QSortFilterProxyModel(parent), m_parent(parent)
    {
        setSortRole(BaseVersionList::SortRole);
        sort(0, Qt::DescendingOrder);
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override
    {
        const auto& filters = m_parent->filters();
        const QString& search = m_parent->search();
        const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);

        if (!search.isEmpty() && !sourceModel()->data(idx, BaseVersionList::VersionRole).toString().contains(search, Qt::CaseInsensitive)) {
            return false;
        }

        for (auto it = filters.begin(); it != filters.end(); ++it) {
            auto data = sourceModel()->data(idx, it.key());
            auto match = data.toString();
            if (!it.value()(match)) {
                return false;
            }
        }
        return true;
    }

    void filterChanged()
    {
#if QT_VERSION < QT_VERSION_CHECK(6, 10, 0)
        invalidateFilter();
#else
        beginFilterChange();
        endFilterChange();
#endif
    }

   private:
    VersionProxyModel* m_parent;
};

VersionProxyModel::VersionProxyModel(QObject* parent) : QAbstractProxyModel(parent), m_filterModel(new VersionFilterModel(this))
{
    connect(m_filterModel, &QAbstractItemModel::dataChanged, this, &VersionProxyModel::sourceDataChanged);
    connect(m_filterModel, &QAbstractItemModel::rowsAboutToBeInserted, this, &VersionProxyModel::sourceRowsAboutToBeInserted);
    connect(m_filterModel, &QAbstractItemModel::rowsInserted, this, &VersionProxyModel::sourceRowsInserted);
    connect(m_filterModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, &VersionProxyModel::sourceRowsAboutToBeRemoved);
    connect(m_filterModel, &QAbstractItemModel::rowsRemoved, this, &VersionProxyModel::sourceRowsRemoved);
    // FIXME: implement when needed
    /*
    connect(replacing, &QAbstractItemModel::rowsAboutToBeMoved, this, &VersionProxyModel::sourceRowsAboutToBeMoved);
    connect(replacing, &QAbstractItemModel::rowsMoved, this, &VersionProxyModel::sourceRowsMoved);
    connect(replacing, &QAbstractItemModel::layoutAboutToBeChanged, this, &VersionProxyModel::sourceLayoutAboutToBeChanged);
    connect(replacing, &QAbstractItemModel::layoutChanged, this, &VersionProxyModel::sourceLayoutChanged);
    */
    connect(m_filterModel, &QAbstractItemModel::modelAboutToBeReset, this, &VersionProxyModel::sourceAboutToBeReset);
    connect(m_filterModel, &QAbstractItemModel::modelReset, this, &VersionProxyModel::sourceReset);

    QAbstractProxyModel::setSourceModel(m_filterModel);
}

QVariant VersionProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section < 0 || section >= m_columns.size()) {
        return {};
    }
    if (orientation != Qt::Horizontal) {
        return {};
    }
    auto column = m_columns[section];
    if (role == Qt::DisplayRole) {
        switch (column) {
            case Name:
                return tr("Version");
            case ParentVersion:
                return tr("Minecraft");  // FIXME: this should come from metadata
            case Branch:
                return tr("Branch");
            case Type:
                return tr("Type");
            case CPUArchitecture:
                return tr("Architecture");
            case Path:
                return tr("Path");
            case JavaName:
                return tr("Java Name");
            case JavaMajor:
                return tr("Major Version");
            case Time:
                return tr("Released");
        }
    } else if (role == Qt::ToolTipRole) {
        switch (column) {
            case Name:
                return tr("The name of the version.");
            case ParentVersion:
                return tr("Minecraft version");  // FIXME: this should come from metadata
            case Branch:
                return tr("The version's branch");
            case Type:
                return tr("The version's type");
            case CPUArchitecture:
                return tr("CPU Architecture");
            case Path:
                return tr("Filesystem path to this version");
            case JavaName:
                return tr("The alternative name of the Java version");
            case JavaMajor:
                return tr("The Java major version");
            case Time:
                return tr("Release date of this version");
        }
    }
    return QVariant();
}

QVariant VersionProxyModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }
    auto column = m_columns[index.column()];
    auto parentIndex = mapToSource(index);
    switch (role) {
        case Qt::DisplayRole: {
            switch (column) {
                case Name: {
                    QString version = sourceModel()->data(parentIndex, BaseVersionList::VersionRole).toString();
                    if (version == m_currentVersion) {
                        return tr("%1 (installed)").arg(version);
                    }
                    return version;
                }
                case ParentVersion:
                    return sourceModel()->data(parentIndex, BaseVersionList::ParentVersionRole);
                case Branch:
                    return sourceModel()->data(parentIndex, BaseVersionList::BranchRole);
                case Type:
                    return sourceModel()->data(parentIndex, BaseVersionList::TypeRole);
                case CPUArchitecture:
                    return sourceModel()->data(parentIndex, BaseVersionList::CPUArchitectureRole);
                case Path:
                    return sourceModel()->data(parentIndex, BaseVersionList::PathRole);
                case JavaName:
                    return sourceModel()->data(parentIndex, BaseVersionList::JavaNameRole);
                case JavaMajor:
                    return sourceModel()->data(parentIndex, BaseVersionList::JavaMajorRole);
                case Time:
                    return sourceModel()->data(parentIndex, Meta::VersionList::TimeRole).toDate();
                default:
                    return QVariant();
            }
        }
        case Qt::ToolTipRole: {
            if (column == Name && m_hasRecommended) {
                auto value = sourceModel()->data(parentIndex, BaseVersionList::RecommendedRole);
                if (value.toBool()) {
                    return tr("Recommended");
                }
                if (m_hasLatest) {
                    auto latest = sourceModel()->data(parentIndex, BaseVersionList::LatestRole);
                    if (latest.toBool()) {
                        return tr("Latest");
                    }
                }
            }
            return sourceModel()->data(parentIndex, BaseVersionList::VersionIdRole);
        }
        case Qt::DecorationRole: {
            if (column == Name && m_hasRecommended) {
                auto recommenced = sourceModel()->data(parentIndex, BaseVersionList::RecommendedRole);
                if (recommenced.toBool()) {
                    return QIcon::fromTheme("star");
                }
                if (m_hasLatest) {
                    auto latest = sourceModel()->data(parentIndex, BaseVersionList::LatestRole);
                    if (latest.toBool()) {
                        return QIcon::fromTheme("bug");
                    }
                }
                QPixmap pixmap;
                QPixmapCache::find("placeholder", &pixmap);
                if (!pixmap) {
                    QPixmap px(16, 16);
                    px.fill(Qt::transparent);
                    QPixmapCache::insert("placeholder", px);
                    return px;
                }
                return pixmap;
            }
            return QVariant();
        }
        default: {
            if (m_roles.contains((BaseVersionList::ModelRoles)role)) {
                return sourceModel()->data(parentIndex, role);
            }
            return QVariant();
        }
    }
}

QModelIndex VersionProxyModel::parent([[maybe_unused]] const QModelIndex& child) const
{
    return QModelIndex();
}

QModelIndex VersionProxyModel::mapFromSource(const QModelIndex& sourceIndex) const
{
    if (sourceIndex.isValid()) {
        return index(sourceIndex.row(), 0);
    }
    return QModelIndex();
}

QModelIndex VersionProxyModel::mapToSource(const QModelIndex& proxyIndex) const
{
    if (proxyIndex.isValid()) {
        return sourceModel()->index(proxyIndex.row(), 0);
    }
    return QModelIndex();
}

QModelIndex VersionProxyModel::index(int row, int column, const QModelIndex& parent) const
{
    // no trees here... shoo
    if (parent.isValid()) {
        return {};
    }
    if (row < 0 || row >= sourceModel()->rowCount()) {
        return {};
    }
    if (column < 0 || column >= columnCount()) {
        return {};
    }
    return QAbstractItemModel::createIndex(row, column);
}

int VersionProxyModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_columns.size());
}

int VersionProxyModel::rowCount(const QModelIndex& parent) const
{
    if (sourceModel()) {
        return sourceModel()->rowCount(parent);
    }
    return 0;
}

void VersionProxyModel::sourceDataChanged(const QModelIndex& sourceTopLeft, const QModelIndex& sourceBottomRight)
{
    if (sourceTopLeft.parent() != sourceBottomRight.parent()) {
        return;
    }

    // whole row is getting changed
    auto topLeft = createIndex(sourceTopLeft.row(), 0);
    auto bottomRight = createIndex(sourceBottomRight.row(), columnCount() - 1);
    emit dataChanged(topLeft, bottomRight);
}

void VersionProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
    auto* replacing = dynamic_cast<BaseVersionList*>(sourceModel);

    m_columns.clear();
    if (!replacing) {
        m_roles.clear();
        m_filterModel->setSourceModel(replacing);
        return;
    }

    m_roles = replacing->providesRoles();
    if (m_roles.contains(BaseVersionList::VersionRole)) {
        m_columns.push_back(Name);
    }
    /*
    if(roles.contains(BaseVersionList::ParentVersionRole))
    {
        m_columns.push_back(ParentVersion);
    }
    */
    if (m_roles.contains(BaseVersionList::CPUArchitectureRole)) {
        m_columns.push_back(CPUArchitecture);
    }
    if (m_roles.contains(BaseVersionList::PathRole)) {
        m_columns.push_back(Path);
    }
    if (m_roles.contains(BaseVersionList::JavaNameRole)) {
        m_columns.push_back(JavaName);
    }
    if (m_roles.contains(BaseVersionList::JavaMajorRole)) {
        m_columns.push_back(JavaMajor);
    }
    if (m_roles.contains(Meta::VersionList::TimeRole)) {
        m_columns.push_back(Time);
    }
    if (m_roles.contains(BaseVersionList::BranchRole)) {
        m_columns.push_back(Branch);
    }
    if (m_roles.contains(BaseVersionList::TypeRole)) {
        m_columns.push_back(Type);
    }
    if (m_roles.contains(BaseVersionList::RecommendedRole)) {
        m_hasRecommended = true;
    }
    if (m_roles.contains(BaseVersionList::LatestRole)) {
        m_hasLatest = true;
    }
    m_filterModel->setSourceModel(replacing);
}

QModelIndex VersionProxyModel::getRecommended() const
{
    if (!m_roles.contains(BaseVersionList::RecommendedRole)) {
        return index(0, 0);
    }
    int recommended = 0;
    for (int i = 0; i < rowCount(); i++) {
        auto value = sourceModel()->data(mapToSource(index(i, 0)), BaseVersionList::RecommendedRole);
        if (value.toBool()) {
            recommended = i;
        }
    }
    return index(recommended, 0);
}

QModelIndex VersionProxyModel::getVersion(const QString& version) const
{
    int found = -1;
    for (int i = 0; i < rowCount(); i++) {
        auto value = sourceModel()->data(mapToSource(index(i, 0)), BaseVersionList::VersionRole);
        if (value.toString() == version) {
            found = i;
        }
    }
    if (found == -1) {
        return QModelIndex();
    }
    return index(found, 0);
}

void VersionProxyModel::clearFilters()
{
    m_filters.clear();
    m_search.clear();
    m_filterModel->filterChanged();
}

void VersionProxyModel::setFilter(const BaseVersionList::ModelRoles column, Filter f)
{
    m_filters[column] = std::move(f);
    m_filterModel->filterChanged();
}

void VersionProxyModel::setSearch(const QString& search)
{
    m_search = search;
    m_filterModel->filterChanged();
}

const VersionProxyModel::FilterMap& VersionProxyModel::filters() const
{
    return m_filters;
}

const QString& VersionProxyModel::search() const
{
    return m_search;
}

void VersionProxyModel::sourceAboutToBeReset()
{
    beginResetModel();
}

void VersionProxyModel::sourceReset()
{
    endResetModel();
}

void VersionProxyModel::sourceRowsAboutToBeInserted(const QModelIndex& parent, int first, int last)
{
    beginInsertRows(parent, first, last);
}

void VersionProxyModel::sourceRowsInserted([[maybe_unused]] const QModelIndex& parent,
                                           [[maybe_unused]] int first,
                                           [[maybe_unused]] int last)
{
    endInsertRows();
}

void VersionProxyModel::sourceRowsAboutToBeRemoved(const QModelIndex& parent, int first, int last)
{
    beginRemoveRows(parent, first, last);
}

void VersionProxyModel::sourceRowsRemoved([[maybe_unused]] const QModelIndex& parent, [[maybe_unused]] int first, [[maybe_unused]] int last)
{
    endRemoveRows();
}

void VersionProxyModel::setCurrentVersion(const QString& version)
{
    m_currentVersion = version;
}

#include "VersionProxyModel.moc"
