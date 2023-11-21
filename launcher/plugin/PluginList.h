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
#pragma once

#include <QAbstractListModel>
#include <QDir>
#include <QList>
#include <QSortFilterProxyModel>

#include "plugin/Plugin.h"
#include "settings/SettingsObject.h"

using PluginId = QString;
using PluginLocator = std::pair<PluginPtr, int>;

class PluginList : public QAbstractListModel {
    Q_OBJECT

   public:
    enum Columns { ActiveColumn = 0, ImageColumn, NameColumn, VersionColumn, DateColumn, NUM_COLUMNS };

    explicit PluginList(SettingsObjectPtr settings, const QString& pluginsDir, QObject* parent = 0);
    virtual ~PluginList();

   public:
    enum PluginListError { NoError = 0, UnknownError };

    void enablePlugins();
    PluginListError loadList();

    [[nodiscard]] QDir dir() const { return QDir(m_pluginsDir); }

    [[nodiscard]] qsizetype size() const { return m_plugins.size(); }
    [[nodiscard]] PluginPtr at(int i) const { return m_plugins.at(i); }

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override {
        return parent.isValid() ? 0 : static_cast<int>(size());
    }
    [[nodiscard]] int columnCount(const QModelIndex& parent = {}) const override {
        return parent.isValid() ? 0 : NUM_COLUMNS;
    }

    [[nodiscard]] bool validateIndex(const QModelIndex& index) const;

    bool setEnabled(const QModelIndexList& indexes, Plugin::EnableAction action);

    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    // bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    class ProxyModel : public QSortFilterProxyModel {
       public:
        explicit ProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

       // protected:
       //  [[nodiscard]] bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
       //  [[nodiscard]] bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;
    };

    QSortFilterProxyModel* createFilterProxyModel(QObject* parent) {
        return new ProxyModel(parent);
    }

   signals:
    void pluginsReloaded();

   private:
    void add(const QList<PluginPtr>& list);
    QList<PluginId> discoverPlugins();
    PluginPtr loadPlugin(const PluginId& id);

    bool m_dirty = false;
    QList<PluginPtr> m_plugins;
    SettingsObjectPtr m_globalSettings;
    QString m_pluginsDir;
    QSet<PluginId> pluginSet;
    bool m_pluginsProbed = false;
};
