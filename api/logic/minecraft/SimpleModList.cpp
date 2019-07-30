/* Copyright 2013-2019 MultiMC Contributors
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

#include "SimpleModList.h"
#include <FileSystem.h>
#include <QMimeData>
#include <QUrl>
#include <QUuid>
#include <QString>
#include <QFileSystemWatcher>
#include <QDebug>

SimpleModList::SimpleModList(const QString &dir) : QAbstractListModel(), m_dir(dir)
{
    FS::ensureFolderPathExists(m_dir.absolutePath());
    m_dir.setFilter(QDir::Readable | QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs |
                    QDir::NoSymLinks);
    m_dir.setSorting(QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);
    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, SIGNAL(directoryChanged(QString)), this, SLOT(directoryChanged(QString)));
}

void SimpleModList::startWatching()
{
    if(is_watching)
        return;

    update();

    is_watching = m_watcher->addPath(m_dir.absolutePath());
    if (is_watching)
    {
        qDebug() << "Started watching " << m_dir.absolutePath();
    }
    else
    {
        qDebug() << "Failed to start watching " << m_dir.absolutePath();
    }
}

void SimpleModList::stopWatching()
{
    if(!is_watching)
        return;

    is_watching = !m_watcher->removePath(m_dir.absolutePath());
    if (!is_watching)
    {
        qDebug() << "Stopped watching " << m_dir.absolutePath();
    }
    else
    {
        qDebug() << "Failed to stop watching " << m_dir.absolutePath();
    }
}

bool SimpleModList::update()
{
    if (!isValid())
        return false;

    QList<Mod> newMods;
    m_dir.refresh();
    for (auto entry : m_dir.entryInfoList())
    {
        newMods.append(Mod(entry));
    }

    beginResetModel();
    mods.swap(newMods);
    endResetModel();

    emit changed();
    return true;
}

void SimpleModList::disableInteraction(bool disabled)
{
    if (interaction_disabled == disabled) {
        return;
    }
    interaction_disabled = disabled;
    if(size()) {
        emit dataChanged(index(0), index(size() - 1));
    }
}

void SimpleModList::directoryChanged(QString path)
{
    update();
}

bool SimpleModList::isValid()
{
    return m_dir.exists() && m_dir.isReadable();
}

// FIXME: this does not take disabled mod (with extra .disable extension) into account...
bool SimpleModList::installMod(const QString &filename)
{
    if(interaction_disabled) {
        return false;
    }

    // NOTE: fix for GH-1178: remove trailing slash to avoid issues with using the empty result of QFileInfo::fileName
    auto originalPath = FS::NormalizePath(filename);
    QFileInfo fileinfo(originalPath);

    if (!fileinfo.exists() || !fileinfo.isReadable())
    {
        qWarning() << "Caught attempt to install non-existing file or file-like object:" << originalPath;
        return false;
    }
    qDebug() << "installing: " << fileinfo.absoluteFilePath();

    Mod installedMod(fileinfo);
    if (!installedMod.valid())
    {
        qDebug() << originalPath << "is not a valid mod. Ignoring it.";
        return false;
    }

    auto type = installedMod.type();
    if (type == Mod::MOD_UNKNOWN)
    {
        qDebug() << "Cannot recognize mod type of" << originalPath << ", ignoring it.";
        return false;
    }

    auto newpath = FS::NormalizePath(FS::PathCombine(m_dir.path(), fileinfo.fileName()));
    if(originalPath == newpath)
    {
        qDebug() << "Overwriting the mod (" << originalPath << ") with itself makes no sense...";
        return false;
    }

    if (type == Mod::MOD_SINGLEFILE || type == Mod::MOD_ZIPFILE || type == Mod::MOD_LITEMOD)
    {
        if(QFile::exists(newpath) || QFile::exists(newpath + QString(".disabled")))
        {
            if(!QFile::remove(newpath))
            {
                // FIXME: report error in a user-visible way
                qWarning() << "Copy from" << originalPath << "to" << newpath << "has failed.";
                return false;
            }
            qDebug() << newpath << "has been deleted.";
        }
        if (!QFile::copy(fileinfo.filePath(), newpath))
        {
            qWarning() << "Copy from" << originalPath << "to" << newpath << "has failed.";
            // FIXME: report error in a user-visible way
            return false;
        }
        FS::updateTimestamp(newpath);
        installedMod.repath(newpath);
        update();
        return true;
    }
    else if (type == Mod::MOD_FOLDER)
    {
        QString from = fileinfo.filePath();
        if(QFile::exists(newpath))
        {
            qDebug() << "Ignoring folder " << from << ", it would merge with " << newpath;
            return false;
        }

        if (!FS::copy(from, newpath)())
        {
            qWarning() << "Copy of folder from" << originalPath << "to" << newpath << "has (potentially partially) failed.";
            return false;
        }
        installedMod.repath(newpath);
        update();
        return true;
    }
    return false;
}

bool SimpleModList::enableMods(const QModelIndexList& indexes, bool enable)
{
    if(interaction_disabled) {
        return false;
    }

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

bool SimpleModList::deleteMods(const QModelIndexList& indexes)
{
    if(interaction_disabled) {
        return false;
    }

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

int SimpleModList::columnCount(const QModelIndex &parent) const
{
    return NUM_COLUMNS;
}

QVariant SimpleModList::data(const QModelIndex &index, int role) const
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

bool SimpleModList::setData(const QModelIndex &index, const QVariant &value, int role)
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

QVariant SimpleModList::headerData(int section, Qt::Orientation orientation, int role) const
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
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
    return QVariant();
}

Qt::ItemFlags SimpleModList::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
    auto flags = defaultFlags;
    if(interaction_disabled) {
        flags &= ~Qt::ItemIsDropEnabled;
    }
    else
    {
        flags |= Qt::ItemIsDropEnabled;
        if(index.isValid()) {
            flags |= Qt::ItemIsUserCheckable;
        }
    }
    return flags;
}

Qt::DropActions SimpleModList::supportedDropActions() const
{
    // copy from outside, move from within and other mod lists
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList SimpleModList::mimeTypes() const
{
    QStringList types;
    types << "text/uri-list";
    return types;
}

bool SimpleModList::dropMimeData(const QMimeData* data, Qt::DropAction action, int, int, const QModelIndex&)
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
        return true;
    }
    return false;
}
