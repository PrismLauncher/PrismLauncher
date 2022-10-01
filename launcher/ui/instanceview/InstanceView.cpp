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

/*
 * Parts of this code were taken from the Dolphin Emulator project, which
 * is licensed under the terms of the GNU General Public License version 2
 * or later.
 */

#include "Application.h"
#include "InstanceView.h"

#include "InstanceList.h"
#include "ui/instanceview/InstanceProxyModel.h"

#include <QHeaderView>
#include <QSize>

InstanceView::InstanceView(QWidget *parent, InstanceList *instances) : QStackedWidget(parent), m_instances(instances) {
    prepareModel();
    createTable();

    addWidget(m_table);
    setCurrentWidget(m_table);
}

void InstanceView::storeState() {
    APPLICATION->settings()->set("InstanceViewTableHeaderState", m_table->horizontalHeader()->saveState().toBase64());
}

void InstanceView::prepareModel() {
    m_proxy = new InstanceProxyModel(this);
    m_proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setSourceModel(m_instances);
    connect(m_proxy, &InstanceProxyModel::dataChanged, this, &InstanceView::dataChanged);
}

void InstanceView::createTable() {

    m_table = new QTableView(this);
    m_table->setModel(m_proxy);

    m_table->setTabKeyNavigation(false);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::EditKeyPressed);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->setSortingEnabled(true);
    m_table->setCurrentIndex(QModelIndex());
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    m_table->setWordWrap(false);
    m_table->setFrameStyle(QFrame::NoFrame);
    m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_table->setIconSize(QSize(48, 48));

    m_table->verticalHeader()->hide();

    QHeaderView *header = m_table->horizontalHeader();
    header->restoreState(QByteArray::fromBase64(APPLICATION->settings()->get("InstanceViewTableHeaderState").toByteArray()));

    header->setSectionsMovable(true);
    header->setSectionResizeMode(InstanceList::Icon, QHeaderView::Fixed);
    header->setSectionResizeMode(InstanceList::Name, QHeaderView::Stretch);
    header->setSectionResizeMode(InstanceList::GameVersion, QHeaderView::Interactive);
    header->setSectionResizeMode(InstanceList::PlayTime, QHeaderView::Interactive);
    header->setSectionResizeMode(InstanceList::LastPlayed, QHeaderView::Interactive);
    m_table->setColumnWidth(InstanceList::Icon, m_rowHeight + 3 + 3);  // padding left and right
    m_table->verticalHeader()->setDefaultSectionSize(m_rowHeight + 1 + 1);  // padding top and bottom

    if (!APPLICATION->settings()->contains("InstanceViewTableHeaderState"))
        m_table->sortByColumn(InstanceList::LastPlayed, Qt::AscendingOrder);

    connect(m_table, &QTableView::doubleClicked, this, &InstanceView::activateInstance);
    connect(m_table->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &InstanceView::currentRowChanged);
    connect(m_table->selectionModel(), &QItemSelectionModel::currentColumnChanged, this, &InstanceView::selectNameColumn);
}

InstancePtr InstanceView::currentInstance() {
    auto current = m_table->selectionModel()->currentIndex();
    if (current.isValid()) {
        int row = mappedIndex(current).row();
        return m_instances->at(row);
    }
    return nullptr;
}

void InstanceView::activateInstance(const QModelIndex &index) {
    if (index.isValid()) {
        int row = mappedIndex(index).row();
        emit instanceActivated(m_instances->at(row));
    }
}

void InstanceView::currentRowChanged(const QModelIndex &current, const QModelIndex &previous) {
    InstancePtr inst1, inst2;
    if (current.isValid()) {
        int row = mappedIndex(current).row();
        inst1 = m_instances->at(row);
    }
    if (previous.isValid()) {
        int row = mappedIndex(previous).row();
        inst2 = m_instances->at(row);
    }
    emit currentInstanceChanged(inst1, inst2);
}

void InstanceView::selectNameColumn(const QModelIndex &current, const QModelIndex &previous) {
    // Make sure Name column is always selected
    m_table->setCurrentIndex(current.siblingAtColumn(InstanceList::Name));
}

void InstanceView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight) {
    // Notify others if data of the current instance changed
    auto current = m_table->selectionModel()->currentIndex();

    QItemSelection foo(topLeft, bottomRight);
    if (foo.contains(current)) {
        int row = mappedIndex(current).row();
        InstancePtr inst = m_instances->at(row);
        emit currentInstanceChanged(inst, inst);
    }
}

QModelIndex InstanceView::mappedIndex(const QModelIndex& index) const {
    return m_proxy->mapToSource(index);
}
