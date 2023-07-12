// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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

#include <QKeyEvent>

#include "Application.h"
#include "EnvironmentVariables.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui_EnvironmentVariables.h"

EnvironmentVariables::EnvironmentVariables(QWidget* parent) : QWidget(parent), ui(new Ui::EnvironmentVariables)
{
    ui->setupUi(this);
    ui->list->installEventFilter(this);

    ui->list->sortItems(0, Qt::AscendingOrder);
    ui->list->setSortingEnabled(true);
    ui->list->header()->resizeSections(QHeaderView::Interactive);
    ui->list->header()->resizeSection(0, 200);

    connect(ui->add, &QPushButton::clicked, this, [this] {
        auto item = new QTreeWidgetItem(ui->list);
        item->setText(0, "ENV_VAR");
        item->setText(1, "value");
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        ui->list->addTopLevelItem(item);
        ui->list->selectionModel()->select(ui->list->model()->index(ui->list->indexOfTopLevelItem(item), 0),
                                           QItemSelectionModel::ClearAndSelect | QItemSelectionModel::SelectionFlag::Rows);
        ui->list->editItem(item);
    });

    connect(ui->remove, &QPushButton::clicked, this, [this] {
        for (QTreeWidgetItem* item : ui->list->selectedItems())
            ui->list->takeTopLevelItem(ui->list->indexOfTopLevelItem(item));
    });

    connect(ui->clear, &QPushButton::clicked, this, [this] { ui->list->clear(); });

    connect(ui->globalOverride, &QCheckBox::clicked, this, [this](bool state) {
        if (!state)
            return;

        auto global = APPLICATION->settings()->get("Env").toMap();
        if (global.isEmpty())
            return;

        auto response = CustomMessageBox::selectable(
                            this, tr("Reset"),
                            tr("You have chosen to ignore global settings.\n\nWould you like to clear the current variables and copy "
                               "the global variables over?"),
                            QMessageBox::Question, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                            ->exec();

        if (response == QMessageBox::Yes)
            initialize(true, checked(), override(), global);
    });
}

EnvironmentVariables::~EnvironmentVariables()
{
    delete ui;
}

void EnvironmentVariables::initialize(bool instance, bool checked, bool override, const QMap<QString, QVariant>& value)
{
    // update widgets to settings
    ui->groupBox->setCheckable(instance);
    ui->groupBox->setChecked(checked);
    ui->globalOverride->setVisible(instance);
    ui->globalOverride->setChecked(override);

    // populate
    ui->list->clear();
    for (auto iter = value.begin(); iter != value.end(); iter++) {
        auto item = new QTreeWidgetItem(ui->list);
        item->setText(0, iter.key());
        item->setText(1, iter.value().toString());
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        ui->list->addTopLevelItem(item);
    }
}

bool EnvironmentVariables::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->list && event->type() == QEvent::KeyPress) {
        const QKeyEvent* keyEvent = (QKeyEvent*)event;
        if (keyEvent->key() == Qt::Key_Delete) {
            emit ui->remove->clicked();
            return true;
        }
    }

    return QObject::eventFilter(watched, event);
}

void EnvironmentVariables::retranslate()
{
    ui->retranslateUi(this);
}

bool EnvironmentVariables::checked() const
{
    if (!ui->groupBox->isCheckable())
        return true;
    return ui->groupBox->isChecked();
}

bool EnvironmentVariables::override() const
{
    if (!ui->globalOverride->isVisible())
        return false;
    return ui->globalOverride->isChecked();
}

QMap<QString, QVariant> EnvironmentVariables::value() const
{
    QMap<QString, QVariant> result;
    QTreeWidgetItem* item = ui->list->topLevelItem(0);
    for (int i = 1; item != nullptr; item = ui->list->topLevelItem(i++))
        result[item->text(0)] = item->text(1);

    return result;
}
