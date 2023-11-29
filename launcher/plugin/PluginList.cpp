// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Mai Lapyst <67418776+Mai-Lapyst@users.noreply.github.com>
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
 */
#include <QDir>
#include <QDirIterator>
#include <QIcon>
#include <QProcess>
#include <QThreadPool>

#include "Application.h"
#include "FileSystem.h"
#include "PluginList.h"
#include "settings/INISettingsObject.h"

PluginList::PluginList(SettingsObjectPtr settings, const QString& pluginsDir, QObject* parent)
    : QAbstractListModel(parent), m_globalSettings(settings)
{
    // Create and normalize path
    if (!QDir::current().exists(pluginsDir)) {
        QDir::current().mkpath(pluginsDir);
    }

    m_pluginsDir = QDir(pluginsDir).canonicalPath();
}

PluginList::~PluginList()
{
    // while (!QThreadPool::globalInstance()->waitForDone(100))
    //     QCoreApplication::processEvents();
}

void PluginList::enablePlugins()
{
    for (auto& plugin : m_plugins) {
        if (plugin->enabled())
            plugin->onEnable();
    }
}

static QMap<QString, PluginLocator> getIdMapping(const QList<PluginPtr>& list)
{
    QMap<QString, PluginLocator> out;
    int i = 0;
    for (auto& item : list) {
        auto id = item->id();
        if (out.contains(id)) {
            qWarning() << "Duplicate ID" << id << "in plugin list";
        }
        out[id] = std::make_pair(item, i);
        i++;
    }
    return out;
}

PluginList::PluginListError PluginList::loadList()
{
    auto existingIds = getIdMapping(m_plugins);
    QList<PluginPtr> newList;

    for (auto& id : discoverPlugins()) {
        if (existingIds.contains(id)) {
            auto instPair = existingIds[id];
            existingIds.remove(id);
        } else {
            PluginPtr instPtr = loadPlugin(id);
            if (instPtr) {
                newList.append(instPtr);
            }
        }
    }

    if (!existingIds.isEmpty()) {
        // TODO: we need to restart when plugins where removed from disk!
    }

    if (newList.size()) {
        add(newList);
    }

    m_dirty = false;

    return NoError;
}

bool PluginList::validateIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return false;

    int row = index.row();
    if (row < 0 || row >= m_plugins.size())
        return false;

    return true;
}

bool PluginList::setEnabled(const QModelIndexList& indexes, Plugin::EnableAction action)
{
    if (indexes.isEmpty())
        return false;

    bool needsRestart = false;
    for (auto const& idx : indexes) {
        if (!validateIndex(idx) || idx.column() != 0)
            continue;

        int row = idx.row();
        auto& plugin = m_plugins[row];

        plugin->enable(action);

        if (plugin->needsRestart()) {
            needsRestart = true;
        }

        emit dataChanged(index(row, 0), index(row, columnCount(QModelIndex()) - 1));
    }

    // reload everything that needs it...
    emit pluginsReloaded();

    return needsRestart;
}

QVariant PluginList::data(const QModelIndex& index, int role) const
{
    if (!validateIndex(index))
        return {};

    int row = index.row();
    int column = index.column();

    switch (role) {
        case Qt::DisplayRole:
            switch (column) {
                case NameColumn:
                    return m_plugins[row]->name();
                case VersionColumn:
                    return m_plugins[row]->version();
                // case DateColumn:
                //     return m_plugins[row]->dateTimeChanged();
                default:
                    return QVariant();
            }
        case Qt::ToolTipRole: {
            if (column == ActiveColumn && m_plugins[row]->needsRestart()) {
                return tr("Warning: This plugin needs the launcher to restart in order to work correctly.");
            }
            return {};
        }
        case Qt::DecorationRole: {
            if (column == ActiveColumn && m_plugins[row]->needsRestart()) {
                return APPLICATION->getThemedIcon("status-yellow");
            } else if (column == ImageColumn) {
                return at(row)->icon({ 32, 32 }, Qt::AspectRatioMode::KeepAspectRatioByExpanding);
            }
            return {};
        }
        case Qt::SizeHintRole:
            if (column == ImageColumn) {
                return QSize(32, 32);
            }
            return {};
        case Qt::CheckStateRole:
            switch (column) {
                case ActiveColumn:
                    return m_plugins[row]->enabled() ? Qt::Checked : Qt::Unchecked;
                default:
                    return {};
            }
        default:
            return {};
    }
}

QVariant PluginList::headerData(int section, [[maybe_unused]] Qt::Orientation orientation, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            switch (section) {
                case ActiveColumn:
                    return tr("Enable");
                case ImageColumn:
                    return tr("Image");
                case NameColumn:
                    return tr("Name");
                case VersionColumn:
                    return tr("Version");
                case DateColumn:
                    return tr("Last Modified");
                default:
                    return QVariant();
            }
        case Qt::ToolTipRole:
            switch (section) {
                case ActiveColumn:
                    return tr("Is the plugin enabled?");
                case NameColumn:
                    return tr("The name of the plugin.");
                case VersionColumn:
                    return tr("The version of the plugin.");
                case DateColumn:
                    return tr("The date and time this plugin was last changed (or added).");
                default:
                    return QVariant();
            }
        default:
            return QVariant();
    }
    return QVariant();
}

// TODO: PluginList::ProxyModel

void PluginList::add(const QList<PluginPtr>& t)
{
    beginInsertRows(QModelIndex(), m_plugins.count(), m_plugins.count() + t.size() - 1);
    m_plugins.append(t);
    endInsertRows();
}

QList<PluginId> PluginList::discoverPlugins()
{
    qDebug() << "Discovering plugins in" << m_pluginsDir;

    QList<QString> out;
    QDirIterator iter(m_pluginsDir, QDir::Dirs | QDir::NoDot | QDir::NoDotDot | QDir::Readable | QDir::Hidden,
                      QDirIterator::FollowSymlinks);

    while (iter.hasNext()) {
        QString subDir = iter.next();
        QFileInfo dirInfo(subDir);
        if (!QFileInfo(FS::PathCombine(subDir, "plugin.json")).exists())
            continue;

        if (dirInfo.isSymLink()) {
            QFileInfo targetInfo(dirInfo.symLinkTarget());
            QFileInfo instDirInfo(m_pluginsDir);
            if (targetInfo.canonicalPath() == instDirInfo.canonicalFilePath()) {
                qDebug() << "Ignoring symlink" << subDir << "that leads into the plugins folder";
                continue;
            }
        }

        auto id = dirInfo.fileName();
        out.append(id);
        qDebug() << "Found plugin ID" << id;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    pluginSet = QSet<QString>(out.begin(), out.end());
#else
    pluginSet = out.toSet();
#endif

    m_pluginsProbed = true;

    return out;
}

PluginPtr PluginList::loadPlugin(const PluginId& id)
{
    auto pluginRoot = FS::PathCombine(m_pluginsDir, id);
    PluginPtr plugin;
    plugin.reset(new Plugin(pluginRoot));
    if (!plugin->loadInfo()) {
        return nullptr;
    }
    qDebug() << "Loaded plugin " << plugin->name() << " from " << plugin->fileinfo();
    return plugin;
}
