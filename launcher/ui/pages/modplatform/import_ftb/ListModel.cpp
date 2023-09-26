// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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
 */

#include "ListModel.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QIcon>
#include <QProcessEnvironment>
#include "FileSystem.h"
#include "StringUtils.h"
#include "modplatform/import_ftb/PackHelpers.h"
#include "ui/widgets/ProjectItem.h"

namespace FTBImportAPP {

QString getPath()
{
    QString partialPath;
#if defined(Q_OS_OSX)
    partialPath = FS::PathCombine(QDir::homePath(), "Library/Application Support");
#elif defined(Q_OS_WIN32)
    partialPath = QProcessEnvironment::systemEnvironment().value("LOCALAPPDATA", "");
#else
    partialPath = QDir::homePath();
#endif
    return FS::PathCombine(partialPath, ".ftba");
}

const QString ListModel::FTB_APP_PATH = getPath();

void ListModel::update()
{
    beginResetModel();
    modpacks.clear();

    QString instancesPath = FS::PathCombine(FTB_APP_PATH, "instances");
    if (auto instancesInfo = QFileInfo(instancesPath); instancesInfo.exists() && instancesInfo.isDir()) {
        QDirIterator directoryIterator(instancesPath, QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable | QDir::Hidden,
                                       QDirIterator::FollowSymlinks);
        while (directoryIterator.hasNext()) {
            auto modpack = parseDirectory(directoryIterator.next());
            if (!modpack.path.isEmpty())
                modpacks.append(modpack);
        }
    } else {
        qDebug() << "Couldn't find ftb instances folder: " << instancesPath;
    }

    endResetModel();
}

QVariant ListModel::data(const QModelIndex& index, int role) const
{
    int pos = index.row();
    if (pos >= modpacks.size() || pos < 0 || !index.isValid()) {
        return QVariant();
    }

    auto pack = modpacks.at(pos);
    if (role == Qt::ToolTipRole) {
    }

    switch (role) {
        case Qt::ToolTipRole:
            return tr("Minecraft %1").arg(pack.mcVersion);
        case Qt::DecorationRole:
            return pack.icon;
        case Qt::UserRole: {
            QVariant v;
            v.setValue(pack);
            return v;
        }
        case Qt::DisplayRole:
            return pack.name;
        case Qt::SizeHintRole:
            return QSize(0, 58);
        // Custom data
        case UserDataTypes::TITLE:
            return pack.name;
        case UserDataTypes::DESCRIPTION:
            return tr("Minecraft %1").arg(pack.mcVersion);
        case UserDataTypes::SELECTED:
            return false;
        case UserDataTypes::INSTALLED:
            return false;
        default:
            break;
    }

    return {};
}

FilterModel::FilterModel(QObject* parent) : QSortFilterProxyModel(parent)
{
    currentSorting = Sorting::ByGameVersion;
    sortings.insert(tr("Sort by Name"), Sorting::ByName);
    sortings.insert(tr("Sort by Game Version"), Sorting::ByGameVersion);
}

bool FilterModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    Modpack leftPack = sourceModel()->data(left, Qt::UserRole).value<Modpack>();
    Modpack rightPack = sourceModel()->data(right, Qt::UserRole).value<Modpack>();

    if (currentSorting == Sorting::ByGameVersion) {
        Version lv(leftPack.mcVersion);
        Version rv(rightPack.mcVersion);
        return lv < rv;

    } else if (currentSorting == Sorting::ByName) {
        return StringUtils::naturalCompare(leftPack.name, rightPack.name, Qt::CaseSensitive) >= 0;
    }

    // UHM, some inavlid value set?!
    qWarning() << "Invalid sorting set!";
    return true;
}

bool FilterModel::filterAcceptsRow([[maybe_unused]] int sourceRow, [[maybe_unused]] const QModelIndex& sourceParent) const
{
    if (searchTerm.isEmpty()) {
        return true;
    }
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    Modpack pack = sourceModel()->data(index, Qt::UserRole).value<Modpack>();
    return pack.name.contains(searchTerm, Qt::CaseInsensitive);
}

void FilterModel::setSearchTerm(const QString term)
{
    searchTerm = term.trimmed();
    invalidate();
}

const QMap<QString, FilterModel::Sorting> FilterModel::getAvailableSortings()
{
    return sortings;
}

QString FilterModel::translateCurrentSorting()
{
    return sortings.key(currentSorting);
}

void FilterModel::setSorting(Sorting s)
{
    currentSorting = s;
    invalidate();
}

FilterModel::Sorting FilterModel::getCurrentSorting()
{
    return currentSorting;
}
}  // namespace FTBImportAPP