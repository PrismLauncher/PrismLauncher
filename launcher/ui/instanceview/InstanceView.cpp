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

#include "InstanceView.h"
#include "Application.h"

#include "InstanceDelegate.h"
#include "InstanceList.h"
#include "ui/instanceview/InstanceGridProxyModel.h"
#include "ui/instanceview/InstanceTableProxyModel.h"

#include <QHeaderView>
#include <QSize>

InstanceView::InstanceView(QWidget* parent, InstanceList* instances) : QStackedWidget(parent), m_instances(instances)
{
    prepareModel();
    createTable();
    createGrid();

    addWidget(m_table);
    addWidget(m_grid);
}

void InstanceView::storeState()
{
    APPLICATION->settings()->set("InstanceViewTableHeaderState", m_table->horizontalHeader()->saveState().toBase64());
}

void InstanceView::switchDisplayMode(InstanceView::DisplayMode mode)
{
    m_displayMode = mode;
    if (mode == DisplayMode::TableMode) {
        setCurrentWidget(m_table);
    } else {
        setCurrentWidget(m_grid);
    }
}

void InstanceView::editSelected()
{
    auto current = currentView()->selectionModel()->currentIndex();
    if (current.isValid()) {
        currentView()->edit(current);
    }
}

void InstanceView::prepareModel()
{
    m_tableProxy = new InstanceTableProxyModel(this);
    m_tableProxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_tableProxy->setSourceModel(m_instances);
    m_tableProxy->setSortRole(InstanceList::SortRole);
    connect(m_tableProxy, &QAbstractItemModel::dataChanged, this, &InstanceView::dataChanged);
    m_gridProxy = new InstanceGridProxyModel(this);
    m_gridProxy->setSourceModel(m_instances);
    m_gridProxy->sort(InstanceList::LastPlayedColumn, Qt::DescendingOrder);
    connect(m_tableProxy, &QAbstractItemModel::dataChanged, this, &InstanceView::dataChanged);
}

void InstanceView::createTable()
{
    m_table = new QTableView(this);
    m_table->setModel(m_tableProxy);
    m_table->setItemDelegate(new InstanceDelegate(this, m_iconSize, false));

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

    QHeaderView* header = m_table->horizontalHeader();

    header->setSectionsMovable(true);
    header->setSectionResizeMode(InstanceList::NameColumn, QHeaderView::Stretch);
    header->setSectionResizeMode(InstanceList::GameVersionColumn, QHeaderView::Interactive);
    header->setSectionResizeMode(InstanceList::PlayTimeColumn, QHeaderView::Interactive);
    header->setSectionResizeMode(InstanceList::LastPlayedColumn, QHeaderView::Interactive);
    m_table->verticalHeader()->setDefaultSectionSize(m_iconSize + 1 + 1);  // padding top and bottom

    if (APPLICATION->settings()->get("InstanceViewTableHeaderState").toString().isEmpty()) {
        m_table->setColumnWidth(InstanceList::GameVersionColumn, 96 + 3 + 3);
        m_table->setColumnWidth(InstanceList::PlayTimeColumn, 96 + 3 + 3);
        m_table->setColumnWidth(InstanceList::LastPlayedColumn, 128 + 3 + 3);
        m_table->sortByColumn(InstanceList::LastPlayedColumn, Qt::DescendingOrder);
    } else {
        header->restoreState(QByteArray::fromBase64(APPLICATION->settings()->get("InstanceViewTableHeaderState").toByteArray()));
    }

    connect(m_table, &QAbstractItemView::doubleClicked, this, &InstanceView::activateInstance);
    connect(m_table->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &InstanceView::currentRowChanged);
    connect(m_table->selectionModel(), &QItemSelectionModel::currentColumnChanged, this, &InstanceView::selectNameColumn);
    connect(m_table, &QWidget::customContextMenuRequested, this, &InstanceView::contextMenuRequested);
}

void InstanceView::createGrid()
{
    m_grid = new QListView(this);
    m_grid->setModel(m_gridProxy);
    m_grid->setModelColumn(InstanceList::NameColumn);
    m_grid->setItemDelegate(new InstanceDelegate(this, m_iconSize, true));

    m_grid->setSelectionMode(QAbstractItemView::SingleSelection);
    m_grid->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_grid->setEditTriggers(QAbstractItemView::EditKeyPressed);
    m_grid->setCurrentIndex(QModelIndex());
    m_grid->setContextMenuPolicy(Qt::CustomContextMenu);
    m_grid->setWordWrap(true);
    m_grid->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_grid->setViewMode(QListView::IconMode);
    m_grid->setMovement(QListView::Static);
    m_grid->setResizeMode(QListView::Adjust);
    m_grid->setFrameStyle(QFrame::NoFrame);
    m_grid->setGridSize(QSize(m_iconSize * 2, m_iconSize * 2));

    connect(m_grid, &QAbstractItemView::doubleClicked, this, &InstanceView::activateInstance);
    connect(m_grid->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &InstanceView::currentRowChanged);
    connect(m_grid, &QWidget::customContextMenuRequested, this, &InstanceView::contextMenuRequested);
}

InstancePtr InstanceView::currentInstance()
{
    auto current = currentView()->selectionModel()->currentIndex();
    if (current.isValid()) {
        int row = mappedIndex(current).row();
        return m_instances->at(row);
    }
    return nullptr;
}

void InstanceView::activateInstance(const QModelIndex& index)
{
    if (index.isValid()) {
        int row = mappedIndex(index).row();
        emit instanceActivated(m_instances->at(row));
    }
}

void InstanceView::currentRowChanged(const QModelIndex& current, const QModelIndex& previous)
{
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

void InstanceView::selectNameColumn(const QModelIndex& current, const QModelIndex& previous)
{
    // Make sure Name column is always selected
    currentView()->setCurrentIndex(current.siblingAtColumn(InstanceList::NameColumn));
}

void InstanceView::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    // Notify others if data of the current instance changed
    auto current = currentView()->selectionModel()->currentIndex();

    QItemSelection foo(topLeft, bottomRight);
    if (foo.contains(current)) {
        int row = mappedIndex(current).row();
        InstancePtr inst = m_instances->at(row);
        emit currentInstanceChanged(inst, inst);
    }
}

void InstanceView::contextMenuRequested(const QPoint pos)
{
    QModelIndex index = currentView()->indexAt(pos);

    if (index.isValid()) {
        int row = mappedIndex(index).row();
        InstancePtr inst = m_instances->at(row);
        emit showContextMenu(currentView()->mapToParent(pos), inst);
    }
}

QModelIndex InstanceView::mappedIndex(const QModelIndex& index) const
{
    if (m_displayMode == DisplayMode::GridMode) {
        return m_gridProxy->mapToSource(index);
    }
    return m_tableProxy->mapToSource(index);
}

void InstanceView::setCatDisplayed(bool enabled)
{
    if (enabled) {
        QDateTime now = QDateTime::currentDateTime();
        QDateTime birthday(QDate(now.date().year(), 12, 28), QTime(0, 0));
        QDateTime xmas(QDate(now.date().year(), 12, 25), QTime(0, 0));
        QString cat;
        if (std::abs(now.daysTo(xmas)) <= 4) {
            cat = "catmas";
        } else if (std::abs(now.daysTo(birthday)) <= 12) {
            cat = "cattiversary";
        } else {
            cat = "kitteh";
        }
        setStyleSheet(QString(R"(
* {
    background-image: url(:/backgrounds/%1);
    background-attachment: fixed;
    background-clip: padding;
    background-position: bottom right;
    background-repeat: none;
    background-color: palette(base);
})")
                          .arg(cat));
        m_table->setAlternatingRowColors(false);
    } else {
        setStyleSheet(QString());
        m_table->setAlternatingRowColors(true);
    }
}
