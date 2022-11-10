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

#include "modplatform/modpacksch/FTBPackManifest.h"

#include "StringUtils.h"

namespace Ftb {

FilterModel::FilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    currentSorting = Sorting::ByPlays;
    sortings.insert(tr("Sort by Plays"), Sorting::ByPlays);
    sortings.insert(tr("Sort by Installs"), Sorting::ByInstalls);
    sortings.insert(tr("Sort by Name"), Sorting::ByName);
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

void FilterModel::setSearchTerm(const QString& term)
{
    searchTerm = term.trimmed();
    invalidate();
}

bool FilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (searchTerm.isEmpty()) {
        return true;
    }

    auto index = sourceModel()->index(sourceRow, 0, sourceParent);
    auto pack = sourceModel()->data(index, Qt::UserRole).value<ModpacksCH::Modpack>();
    return pack.name.contains(searchTerm, Qt::CaseInsensitive);
}

bool FilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    ModpacksCH::Modpack leftPack = sourceModel()->data(left, Qt::UserRole).value<ModpacksCH::Modpack>();
    ModpacksCH::Modpack rightPack = sourceModel()->data(right, Qt::UserRole).value<ModpacksCH::Modpack>();

    if (currentSorting == ByPlays) {
        return leftPack.plays < rightPack.plays;
    }
    else if (currentSorting == ByInstalls) {
        return leftPack.installs < rightPack.installs;
    }
    else if (currentSorting == ByName) {
        return StringUtils::naturalCompare(leftPack.name, rightPack.name, Qt::CaseSensitive) >= 0;
    }

    // Invalid sorting set, somehow...
    qWarning() << "Invalid sorting set!";
    return true;
}

}
