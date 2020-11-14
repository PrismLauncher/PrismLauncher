#include "FtbFilterModel.h"

#include <QDebug>

#include "modplatform/modpacksch/FTBPackManifest.h"
#include <MMCStrings.h>

namespace Ftb {

FilterModel::FilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    currentSorting = Sorting::ByPlays;
    sortings.insert(tr("Sort by plays"), Sorting::ByPlays);
    sortings.insert(tr("Sort by installs"), Sorting::ByInstalls);
    sortings.insert(tr("Sort by name"), Sorting::ByName);
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

bool FilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    return true;
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
        return Strings::naturalCompare(leftPack.name, rightPack.name, Qt::CaseSensitive) >= 0;
    }

    // Invalid sorting set, somehow...
    qWarning() << "Invalid sorting set!";
    return true;
}

}
