// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (C) 2023 Tayou <tayou@gmx.net>
 *  Copyright (C) 2023 TheLastRar <TheLastRar@users.noreply.github.com>
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
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "GameOptionsPage.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/gameoptions/GameOptionDelegate.h"
#include "minecraft/gameoptions/GameOptions.h"
#include "ui_GameOptionsPage.h"

GameOptionsPage::GameOptionsPage(MinecraftInstance* inst, QWidget* parent) : QWidget(parent), ui(new Ui::GameOptionsPage)
{
    ui->setupUi(this);
    ui->tabWidget->tabBar()->hide();
    m_model = inst->gameOptionsModel();
    ui->optionsView->setModel(m_model.get());
    ui->optionsView->setItemDelegateForColumn(2, new GameOptionDelegate(ui->optionsView, m_model->getContents()));
    ui->optionsView->setEditTriggers(QAbstractItemView::AllEditTriggers);
    for (int i = 0; i < m_model->getContents()->size(); ++i) {
        ui->optionsView->openPersistentEditor(m_model->index(i, 2));
    }
    auto head = ui->optionsView->header();
    head->setDefaultSectionSize(350);
    if (head->count()) {
        for (int i = 1; i < head->count(); i++) {
            head->setSectionResizeMode(i, QHeaderView::Interactive);
        }
        head->setSectionResizeMode(head->count() - 1, QHeaderView::Stretch);
    }
    connect(ui->optionsView, &QTreeView::doubleClicked, this, &GameOptionsPage::OptionDoubleClicked);
}

GameOptionsPage::~GameOptionsPage()
{
    // m_model->save();
}

void GameOptionsPage::openedImpl()
{
    // m_model->observe();
}

void GameOptionsPage::closedImpl()
{
    // m_model->unobserve();
}

void GameOptionsPage::retranslate()
{
    ui->retranslateUi(this);
}

// QTreeView's double click checks if the cell clicked on has children
// but a typical tree model would only have children in the first column
// Workaround this by calling expand ourself
void GameOptionsPage::OptionDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid() || index.column() == 0)
        return;

    const QModelIndex firstColumn = ui->optionsView->model()->index(index.row(), 0, index.parent());
    if (!ui->optionsView->model()->hasChildren(firstColumn))
        return;

    if (ui->optionsView->isExpanded(firstColumn))
        ui->optionsView->collapse(firstColumn);
    else
        ui->optionsView->expand(firstColumn);
}
