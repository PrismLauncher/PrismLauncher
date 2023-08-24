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

#include "OptionalModDialog.h"
#include "ui_OptionalModDialog.h"

OptionalModListModel::OptionalModListModel(QWidget* parent, QStringList mods) : QAbstractListModel(parent), m_mods(mods) {}

QStringList OptionalModListModel::getResult()
{
    QStringList result;
    for (const auto& mod : m_mods) {
        if (m_selected.value(mod, false)) {
            result << mod;
        }
    }
    return result;
}

int OptionalModListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_mods.size();
}

int OptionalModListModel::columnCount(const QModelIndex& parent) const
{
    // Enabled, Name
    return parent.isValid() ? 0 : 2;
}

QVariant OptionalModListModel::data(const QModelIndex& index, int role) const
{
    auto row = index.row();
    auto mod = m_mods.at(row);

    if (role == Qt::DisplayRole && index.column() == NameColumn) {
        return mod;
    } else if (role == Qt::CheckStateRole && index.column() == EnabledColumn) {
        return m_selected.value(mod, false) ? Qt::Checked : Qt::Unchecked;
    }

    return {};
}

bool OptionalModListModel::setData(const QModelIndex& index, [[maybe_unused]] const QVariant& value, int role)
{
    if (role == Qt::CheckStateRole) {
        auto row = index.row();
        auto mod = m_mods.at(row);

        toggleMod(mod, row);
        return true;
    }

    return false;
}

QVariant OptionalModListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case EnabledColumn:
                return QString();
            case NameColumn:
                return QString("Name");
        }
    }

    return {};
}

Qt::ItemFlags OptionalModListModel::flags(const QModelIndex& index) const
{
    auto flags = QAbstractListModel::flags(index);
    if (index.isValid() && index.column() == EnabledColumn) {
        flags |= Qt::ItemIsUserCheckable;
    }
    return flags;
}

void OptionalModListModel::toggleAll(bool enabled)
{
    for (const auto& mod : m_mods) {
        m_selected[mod] = enabled;
    }

    emit dataChanged(OptionalModListModel::index(0, EnabledColumn), OptionalModListModel::index(m_mods.size() - 1, EnabledColumn));
}

void OptionalModListModel::toggleMod(QString mod, int index)
{
    auto enable = !m_selected.value(mod, false);

    setMod(mod, index, enable);
}

void OptionalModListModel::setMod(QString mod, int index, bool enable, bool shouldEmit)
{
    if (m_selected.value(mod, false) == enable)
        return;

    m_selected[mod] = enable;

    if (shouldEmit) {
        emit dataChanged(OptionalModListModel::index(index, EnabledColumn), OptionalModListModel::index(index, EnabledColumn));
    }
}

OptionalModDialog::OptionalModDialog(QWidget* parent, QStringList mods) : QDialog(parent), ui(new Ui::OptionalModDialog)
{
    ui->setupUi(this);

    listModel = new OptionalModListModel(this, mods);
    ui->treeView->setModel(listModel);

    ui->treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->treeView->header()->setSectionResizeMode(OptionalModListModel::NameColumn, QHeaderView::Stretch);

    connect(ui->selectAllButton, &QPushButton::clicked, listModel, &OptionalModListModel::selectAll);
    connect(ui->clearAllButton, &QPushButton::clicked, listModel, &OptionalModListModel::clearAll);
    connect(ui->installButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

OptionalModDialog::~OptionalModDialog()
{
    delete ui;
}
