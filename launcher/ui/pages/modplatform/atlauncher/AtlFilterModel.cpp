#include "AtlFilterModel.h"

#include <QDebug>

#include <modplatform/atlauncher/ATLPackIndex.h>
#include <Version.h>
#include <MMCStrings.h>

namespace Atl {

FilterModel::FilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    currentSorting = Sorting::ByPopularity;
    sortings.insert(tr("Sort by popularity"), Sorting::ByPopularity);
    sortings.insert(tr("Sort by name"), Sorting::ByName);
    sortings.insert(tr("Sort by game version"), Sorting::ByGameVersion);

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

bool FilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (searchTerm.isEmpty()) {
        return true;
    }

    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    ATLauncher::IndexedPack pack = sourceModel()->data(index, Qt::UserRole).value<ATLauncher::IndexedPack>();
    return pack.name.contains(searchTerm, Qt::CaseInsensitive);
}

bool FilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    ATLauncher::IndexedPack leftPack = sourceModel()->data(left, Qt::UserRole).value<ATLauncher::IndexedPack>();
    ATLauncher::IndexedPack rightPack = sourceModel()->data(right, Qt::UserRole).value<ATLauncher::IndexedPack>();

    if (currentSorting == ByPopularity) {
        return leftPack.position > rightPack.position;
    }
    else if (currentSorting == ByGameVersion) {
        Version lv(leftPack.versions.at(0).minecraft);
        Version rv(rightPack.versions.at(0).minecraft);
        return lv < rv;
    }
    else if (currentSorting == ByName) {
        return Strings::naturalCompare(leftPack.name, rightPack.name, Qt::CaseSensitive) >= 0;
    }

    // Invalid sorting set, somehow...
    qWarning() << "Invalid sorting set!";
    return true;
}

}
