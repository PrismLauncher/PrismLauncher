// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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
#pragma once

#include <QAbstractItemView>
#include <QListView>
#include <QStackedWidget>
#include <QTableView>

#include "BaseInstance.h"
#include "InstanceList.h"

class InstanceTableProxyModel;
class InstanceGridProxyModel;

class InstanceView : public QStackedWidget {
    Q_OBJECT

   public:
    enum DisplayMode { TableMode = 0, GridMode };

    explicit InstanceView(QWidget* parent = nullptr, InstanceList* instances = nullptr);

    void switchDisplayMode(DisplayMode mode);

    QAbstractItemView* currentView()
    {
        if (m_displayMode == GridMode)
            return m_grid;
        return m_table;
    }

    InstancePtr currentInstance();

    // save state of current view
    void storeState();

    void setCatDisplayed(bool enabled);

    void editSelected(InstanceList::Column targetColumn = InstanceList::NameColumn);

   signals:
    void instanceActivated(InstancePtr inst);
    void currentInstanceChanged(InstancePtr current, InstancePtr previous);
    void showContextMenu(const QPoint pos, InstancePtr inst);

   private slots:
    void activateInstance(const QModelIndex& index);
    void currentRowChanged(const QModelIndex& current, const QModelIndex& previous);
    void selectNameColumn(const QModelIndex& current, const QModelIndex& previous);
    // emits currentRowChanged if a data update affected the current instance
    void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
    void contextMenuRequested(const QPoint pos);

   private:
    void createTable();
    void createGrid();
    void prepareModel();
    QModelIndex mappedIndex(const QModelIndex& index) const;

    int m_iconSize = 48;
    DisplayMode m_displayMode = TableMode;

    QTableView* m_table;
    QListView* m_grid;
    InstanceTableProxyModel* m_tableProxy;
    InstanceGridProxyModel* m_gridProxy;
    InstanceList* m_instances;
};
