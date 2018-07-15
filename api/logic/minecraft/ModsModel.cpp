/* Copyright 2013-2018 MultiMC Contributors
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

#include "ModsModel.h"
#include <FileSystem.h>
#include <QMimeData>
#include <QUrl>
#include <QUuid>
#include <QString>
#include <QFileSystemWatcher>
#include <QDebug>

ModsModel::ModsModel(const QString &mainDir, const QString &coreDir, const QString &cacheLocation)
    :QAbstractListModel(), m_mainDir(mainDir), m_coreDir(coreDir)
{
    FS::ensureFolderPathExists(m_mainDir.absolutePath());
    m_mainDir.setFilter(QDir::Readable | QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs |
                        QDir::NoSymLinks);
    m_mainDir.setSorting(QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);
    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, SIGNAL(directoryChanged(QString)), this, SLOT(directoryChanged(QString)));
}

void ModsModel::startWatching()
{
    if(is_watching)
        return;

    update();

    is_watching = m_watcher->addPath(m_mainDir.absolutePath());
    if (is_watching)
    {
        qDebug() << "Started watching " << m_mainDir.absolutePath();
    }
    else
    {
        qDebug() << "Failed to start watching " << m_mainDir.absolutePath();
    }
}

void ModsModel::stopWatching()
{
    if(!is_watching)
        return;

    is_watching = !m_watcher->removePath(m_mainDir.absolutePath());
    if (!is_watching)
    {
        qDebug() << "Stopped watching " << m_mainDir.absolutePath();
    }
    else
    {
        qDebug() << "Failed to stop watching " << m_mainDir.absolutePath();
    }
}

bool ModsModel::update()
{
    if (!isValid())
        return false;

    QList<Mod> orderedMods;
    QList<Mod> newMods;
    m_mainDir.refresh();
    auto folderContents = m_mainDir.entryInfoList();
    bool orderOrStateChanged = false;

    // if there are any untracked files...
    if (folderContents.size())
    {
        // the order surely changed!
        for (auto entry : folderContents)
        {
            newMods.append(Mod(entry));
        }
        orderedMods.append(newMods);
        orderOrStateChanged = true;
    }
    // otherwise, if we were already tracking some mods
    else if (mods.size())
    {
        // if the number doesn't match, order changed.
        if (mods.size() != orderedMods.size())
            orderOrStateChanged = true;
        // if it does match, compare the mods themselves
        else
            for (int i = 0; i < mods.size(); i++)
            {
                if (!mods[i].strongCompare(orderedMods[i]))
                {
                    orderOrStateChanged = true;
                    break;
                }
            }
    }
    beginResetModel();
    mods.swap(orderedMods);
    endResetModel();
    if (orderOrStateChanged)
    {
        emit changed();
    }
    return true;
}

void ModsModel::directoryChanged(QString path)
{
    update();
}

bool ModsModel::isValid()
{
    return m_mainDir.exists() && m_mainDir.isReadable();
}

bool ModsModel::installMod(const QString &filename)
{
    // NOTE: fix for GH-1178: remove trailing slash to avoid issues with using the empty result of QFileInfo::fileName
    QFileInfo fileinfo(FS::NormalizePath(filename));

    qDebug() << "installing: " << fileinfo.absoluteFilePath();

    if (!fileinfo.exists() || !fileinfo.isReadable())
    {
        return false;
    }
    Mod m(fileinfo);
    if (!m.valid())
        return false;

    auto type = m.type();
    if (type == Mod::MOD_UNKNOWN)
        return false;
    if (type == Mod::MOD_SINGLEFILE || type == Mod::MOD_ZIPFILE || type == Mod::MOD_LITEMOD)
    {
        QString newpath = FS::PathCombine(m_mainDir.path(), fileinfo.fileName());
        if (!QFile::copy(fileinfo.filePath(), newpath))
            return false;
        FS::updateTimestamp(newpath);
        m.repath(newpath);
        update();
        return true;
    }
    else if (type == Mod::MOD_FOLDER)
    {
        QString from = fileinfo.filePath();
        QString to = FS::PathCombine(m_mainDir.path(), fileinfo.fileName());
        if (!FS::copy(from, to)())
            return false;
        m.repath(to);
        update();
        return true;
    }
    return false;
}

bool ModsModel::enableMods(const QModelIndexList& indexes, bool enable)
{
    if(indexes.isEmpty())
        return true;

    for (auto i: indexes)
    {
        Mod &m = mods[i.row()];
        m.enable(enable);
        emit dataChanged(i, i);
    }
    emit changed();
    return true;
}

bool ModsModel::deleteMods(const QModelIndexList& indexes)
{
    if(indexes.isEmpty())
        return true;

    for (auto i: indexes)
    {
        Mod &m = mods[i.row()];
        m.destroy();
    }
    emit changed();
    return true;
}

int ModsModel::columnCount(const QModelIndex &parent) const
{
    return NUM_COLUMNS;
}

QVariant ModsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int row = index.row();
    int column = index.column();

    if (row < 0 || row >= mods.size())
        return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
        switch (column)
        {
        case NameColumn:
            return mods[row].name();
        case VersionColumn:
            return mods[row].version();
        case DateColumn:
            return mods[row].dateTimeChanged();
        case LocationColumn:
            return "Unknown";

        default:
            return QVariant();
        }

    case Qt::ToolTipRole:
        return mods[row].mmc_id();

    case Qt::CheckStateRole:
        switch (column)
        {
        case ActiveColumn:
            return mods[row].enabled() ? Qt::Checked : Qt::Unchecked;
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

bool ModsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.row() < 0 || index.row() >= rowCount(index) || !index.isValid())
    {
        return false;
    }

    if (role == Qt::CheckStateRole)
    {
        auto &mod = mods[index.row()];
        if (mod.enable(!mod.enabled()))
        {
            emit dataChanged(index, index);
            return true;
        }
    }
    return false;
}

QVariant ModsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (role)
    {
    case Qt::DisplayRole:
        switch (section)
        {
        case ActiveColumn:
            return QString();
        case NameColumn:
            return tr("Name");
        case VersionColumn:
            return tr("Version");
        case DateColumn:
            return tr("Last changed");
        case LocationColumn:
            return tr("Location");
        default:
            return QVariant();
        }

    case Qt::ToolTipRole:
        switch (section)
        {
        case ActiveColumn:
            return tr("Is the mod enabled?");
        case NameColumn:
            return tr("The name of the mod.");
        case VersionColumn:
            return tr("The version of the mod.");
        case DateColumn:
            return tr("The date and time this mod was last changed (or added).");
        case LocationColumn:
            return tr("Where the mod is located (inside or outside the instance).");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
    return QVariant();
}

Qt::ItemFlags ModsModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
    if (index.isValid())
        return Qt::ItemIsUserCheckable | Qt::ItemIsDropEnabled |
               defaultFlags;
    else
        return Qt::ItemIsDropEnabled | defaultFlags;
}

Qt::DropActions ModsModel::supportedDropActions() const
{
    // copy from outside, move from within and other mod lists
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList ModsModel::mimeTypes() const
{
    QStringList types;
    types << "text/uri-list";
    return types;
}

bool ModsModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int, int, const QModelIndex&)
{
    if (action == Qt::IgnoreAction)
    {
        return true;
    }

    // check if the action is supported
    if (!data || !(action & supportedDropActions()))
    {
        return false;
    }

    // files dropped from outside?
    if (data->hasUrls())
    {
        bool was_watching = is_watching;
        if (was_watching)
        {
            stopWatching();
        }
        auto urls = data->urls();
        for (auto url : urls)
        {
            // only local files may be dropped...
            if (!url.isLocalFile())
            {
                continue;
            }
            // TODO: implement not only copy, but also move
            // FIXME: handle errors here
            installMod(url.toLocalFile());
        }
        if (was_watching)
        {
            startWatching();
        }
        return true;
    }
    return false;
}
