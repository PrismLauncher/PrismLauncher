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

#include "AtlFilterModel.h"

#include <QDebug>

#include <Version.h>
#include <modplatform/atlauncher/ATLPackIndex.h>

#include "StringUtils.h"

namespace Atl {

FilterModel::FilterModel(QObject* parent) : QSortFilterProxyModel(parent)
{
    currentSorting = Sorting::ByPopularity;
    sortings.insert(tr("Sort by Popularity"), Sorting::ByPopularity);
    sortings.insert(tr("Sort by Name"), Sorting::ByName);
    sortings.insert(tr("Sort by Game Version"), Sorting::ByGameVersion);

    searchTerm = "";
}

const QMap<QString, FilterModel::Sorting> FilterModel::getAvailableSortings()
{
    return sortings;
}

QString FilterModel::translateCurrentSorting()
{
    return sortings.key(currentSorting);
}

void FilterModel::setSorting(Sorting sorting)
{
    currentSorting = sorting;
    invalidate();
}

FilterModel::Sorting FilterModel::getCurrentSorting()
{
    return currentSorting;
}

void FilterModel::setSearchTerm(const QString term)
{
    searchTerm = term.trimmed();
    invalidate();
}

bool FilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if (searchTerm.isEmpty()) {
        return true;
    }
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    ATLauncher::IndexedPack pack = sourceModel()->data(index, Qt::UserRole).value<ATLauncher::IndexedPack>();
    if (searchTerm.startsWith("#"))
        return QString::number(pack.id) == searchTerm.mid(1);
    return pack.name.contains(searchTerm, Qt::CaseInsensitive);
}

bool FilterModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    ATLauncher::IndexedPack leftPack = sourceModel()->data(left, Qt::UserRole).value<ATLauncher::IndexedPack>();
    ATLauncher::IndexedPack rightPack = sourceModel()->data(right, Qt::UserRole).value<ATLauncher::IndexedPack>();

    if (currentSorting == ByPopularity) {
        return leftPack.position > rightPack.position;
    } else if (currentSorting == ByGameVersion) {
        Version lv(leftPack.versions.at(0).minecraft);
        Version rv(rightPack.versions.at(0).minecraft);
        return lv < rv;
    } else if (currentSorting == ByName) {
        return StringUtils::naturalCompare(leftPack.name, rightPack.name, Qt::CaseSensitive) >= 0;
    }

    // Invalid sorting set, somehow...
    qWarning() << "Invalid sorting set!";
    return true;
}

}  // namespace Atl
