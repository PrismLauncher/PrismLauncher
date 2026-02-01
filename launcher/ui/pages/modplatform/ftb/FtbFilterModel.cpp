/*
 * Copyright 2020-2021 Jamie Mansfield <jmansfield@cadixdev.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "FtbFilterModel.h"

#include <QDebug>

#include "modplatform/ftb/FTBPackManifest.h"

#include "StringUtils.h"

namespace Ftb {

FilterModel::FilterModel(QObject* parent) : QSortFilterProxyModel(parent)
{
    m_currentSorting = Sorting::ByPlays;
    m_sortings.insert(tr("Sort by Plays"), Sorting::ByPlays);
    m_sortings.insert(tr("Sort by Installs"), Sorting::ByInstalls);
    m_sortings.insert(tr("Sort by Name"), Sorting::ByName);
}

const QMap<QString, FilterModel::Sorting> FilterModel::getAvailableSortings()
{
    return m_sortings;
}

QString FilterModel::translateCurrentSorting()
{
    return m_sortings.key(m_currentSorting);
}

void FilterModel::setSorting(Sorting sorting)
{
    m_currentSorting = sorting;
    invalidate();
}

FilterModel::Sorting FilterModel::getCurrentSorting()
{
    return m_currentSorting;
}

void FilterModel::setSearchTerm(const QString& term)
{
    m_searchTerm = term.trimmed();
    invalidate();
}

bool FilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if (m_searchTerm.isEmpty()) {
        return true;
    }

    auto index = sourceModel()->index(sourceRow, 0, sourceParent);
    auto pack = sourceModel()->data(index, Qt::UserRole).value<FTB::Modpack>();
    return pack.name.contains(m_searchTerm, Qt::CaseInsensitive);
}

bool FilterModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    FTB::Modpack leftPack = sourceModel()->data(left, Qt::UserRole).value<FTB::Modpack>();
    FTB::Modpack rightPack = sourceModel()->data(right, Qt::UserRole).value<FTB::Modpack>();

    if (m_currentSorting == ByPlays) {
        return leftPack.plays < rightPack.plays;
    } else if (m_currentSorting == ByInstalls) {
        return leftPack.installs < rightPack.installs;
    } else if (m_currentSorting == ByName) {
        return StringUtils::naturalCompare(leftPack.name, rightPack.name, Qt::CaseSensitive) >= 0;
    }

    // Invalid sorting set, somehow...
    qWarning() << "Invalid sorting set!";
    return true;
}

}  // namespace Ftb
