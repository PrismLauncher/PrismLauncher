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

#include "InstancesView.h"
#include "Application.h"

#include "InstanceDelegate.h"
#include "InstanceList.h"
#include "icons/IconImageProvider.h"
#include "ui/instanceview/InstanceGridProxyModel.h"
#include "ui/instanceview/InstanceTableProxyModel.h"

#include <QHeaderView>
#include <QKeyEvent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QSize>
#include <QSortFilterProxyModel>

InstancesView::InstancesView(QWidget* parent, InstanceList* instances) : QStackedWidget(parent), m_instances(instances)
{
    prepareModel();
    createTable();
    createGrid();

    addWidget(m_table);
    addWidget(m_grid);
}

void InstancesView::storeState()
{
    APPLICATION->settings()->set("InstanceViewTableHeaderState", m_table->horizontalHeader()->saveState().toBase64());
}

void InstancesView::switchDisplayMode(InstancesView::DisplayMode mode)
{
    const QModelIndex index = currentView()->currentIndex();
    const QModelIndex sourceIndex = mappedIndex(index);
    if (mode == DisplayMode::TableMode) {
        m_table->selectionModel()->setCurrentIndex(m_tableProxy->mapFromSource(sourceIndex),
                                                   QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        setCurrentWidget(m_table);
    } else {
        // m_grid->selectionModel()->setCurrentIndex(m_gridProxy->mapFromSource(sourceIndex),
        //                                           QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        setCurrentWidget(m_grid);
    }
    m_displayMode = mode;
}

void InstancesView::editSelected(InstanceList::Column targetColumn)
{
    auto current = currentView()->selectionModel()->currentIndex();
    if (current.isValid()) {
        currentView()->edit(current.siblingAtColumn(targetColumn));
    }
}

void InstancesView::prepareModel()
{
    m_tableProxy = new InstanceTableProxyModel(this);
    m_tableProxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_tableProxy->setSourceModel(m_instances);
    m_tableProxy->setSortRole(InstanceList::SortRole);
    connect(m_tableProxy, &QAbstractItemModel::dataChanged, this, &InstancesView::dataChanged);
    m_gridProxy = new InstanceGridProxyModel(this);
    m_gridProxy->setSourceModel(m_instances);
    m_gridProxy->sort(InstanceList::LastPlayedColumn, Qt::DescendingOrder);
    connect(m_tableProxy, &QAbstractItemModel::dataChanged, this, &InstancesView::dataChanged);
}

void InstancesView::createTable()
{
    m_table = new QTableView(this);
    m_table->installEventFilter(this);
    m_table->setModel(m_tableProxy);
    m_table->setItemDelegate(new InstanceDelegate(this, m_iconSize));

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

    m_table->setColumnHidden(InstanceList::StatusColumn, true);

    connect(m_table, &QAbstractItemView::doubleClicked, this, &InstancesView::activateInstance);
    connect(m_table->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &InstancesView::currentRowChanged);
    connect(m_table->selectionModel(), &QItemSelectionModel::currentColumnChanged, this, &InstancesView::selectNameColumn);
    connect(m_table, &QWidget::customContextMenuRequested, this, &InstancesView::contextMenuRequested);
}

void InstancesView::createGrid()
{
    m_grid = new QQuickWidget(this);
    m_grid->rootContext()->setContextProperty("instances", m_gridProxy);
    m_grid->rootContext()->setContextProperty("iconSize", m_iconSize);
    m_grid->engine()->addImageProvider("instance", new IconImageProvider(APPLICATION->icons(), m_iconSize));
    m_grid->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_grid->setSource(QUrl("qrc:/instanceview/InstancesGrid.qml"));
    m_grid->installEventFilter(this);
    m_grid->setContextMenuPolicy(Qt::CustomContextMenu);
    m_grid->show();

    // connect(m_grid, &QAbstractItemView::doubleClicked, this, &InstancesView::activateInstance);
    // connect(m_grid->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &InstancesView::currentRowChanged);
    connect(m_grid, &QWidget::customContextMenuRequested, this, &InstancesView::contextMenuRequested);
}

InstancePtr InstancesView::currentInstance()
{
    auto current = currentView()->selectionModel()->currentIndex();
    if (current.isValid()) {
        int row = mappedIndex(current).row();
        return m_instances->at(row);
    }
    return nullptr;
}

bool InstancesView::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        switch (keyEvent->key()) {
            case Qt::Key_Return:
            case Qt::Key_Enter:
                activateInstance(currentView()->selectionModel()->currentIndex());
                return true;
            case Qt::Key_F5:
                emit refreshInstances();
                return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

void InstancesView::activateInstance(const QModelIndex& index)
{
    if (index.isValid()) {
        int row = mappedIndex(index).row();
        emit instanceActivated(m_instances->at(row));
    }
}

void InstancesView::currentRowChanged(const QModelIndex& current, const QModelIndex& previous)
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

void InstancesView::selectNameColumn(const QModelIndex& current, const QModelIndex& previous)
{
    // Make sure Name column is always selected
    if (current.column() != InstanceList::NameColumn && current.column() != InstanceList::CategoryColumn)
        currentView()->setCurrentIndex(current.siblingAtColumn(InstanceList::NameColumn));
}

void InstancesView::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
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

void InstancesView::contextMenuRequested(const QPoint pos)
{
    QModelIndex index = currentView()->indexAt(pos);

    if (index.isValid()) {
        int row = mappedIndex(index).row();
        InstancePtr inst = m_instances->at(row);
        emit showContextMenu(currentView()->mapToParent(pos), inst);
    }
}

QModelIndex InstancesView::mappedIndex(const QModelIndex& index) const
{
    if (index.model() == m_tableProxy)
        return m_tableProxy->mapToSource(index);
    if (index.model() == m_gridProxy)
        return m_gridProxy->mapToSource(index);
    return index;
}

void InstancesView::setFilterQuery(const QString& query)
{
    m_gridProxy->setFilterQuery(query);
    m_tableProxy->setFilterQuery(query);
}

void InstancesView::setCatDisplayed(bool enabled)
{
    if (enabled) {
        QDateTime now = QDateTime::currentDateTime();
        QDateTime xmas(QDate(now.date().year(), 12, 25), QTime(0, 0));
        QDateTime halloween(QDate(now.date().year(), 10, 31), QTime(0, 0));
        QDateTime birthday(QDate(now.date().year(), 10, 17), QTime(00, 0));
        QString cat = APPLICATION->settings()->get("BackgroundCat").toString();
        if (std::abs(now.daysTo(xmas)) <= 4) {
            cat += "-xmas";
        } else if (std::abs(now.daysTo(halloween)) <= 12) {
            cat += "-spooky";
        } else if (std::abs(now.daysTo(birthday)) <= 12) {
            cat += "-bday";
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
