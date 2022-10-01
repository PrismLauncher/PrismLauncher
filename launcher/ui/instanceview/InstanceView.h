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
#include <QStackedWidget>
#include <QTableView>

#include "BaseInstance.h"

class InstanceProxyModel;
class InstanceList;

class InstanceView : public QStackedWidget {
    Q_OBJECT

   public:
    explicit InstanceView(QWidget* parent = nullptr, InstanceList* instances = nullptr);

    QAbstractItemView* currentView() { return m_table; }

    InstancePtr currentInstance();

    // save state of current view
    void storeState();

    void setCatDisplayed(bool enabled);

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
    void prepareModel();
    QModelIndex mappedIndex(const QModelIndex& index) const;

    int m_rowHeight = 48;

    QTableView* m_table;
    InstanceProxyModel* m_proxy;
    InstanceList* m_instances;
};
