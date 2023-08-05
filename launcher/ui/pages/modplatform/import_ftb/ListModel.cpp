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
#include "modplatform/import_ftb/PackHelpers.h"

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
    if (role == Qt::DisplayRole) {
        return pack.name;
    } else if (role == Qt::DecorationRole) {
        return pack.icon;
    } else if (role == Qt::UserRole) {
        QVariant v;
        v.setValue(pack);
        return v;
    } else if (role == Qt::ToolTipRole) {
        return tr("Minecraft %1").arg(pack.mcVersion);
    }

    return QVariant();
}
}  // namespace FTBImportAPP