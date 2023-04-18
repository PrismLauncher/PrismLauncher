// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Tayou <tayou@gmx.net>
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

#include "GameOptionWidgetSpinnerFloat.h"
#include "ui_GameOptionWidgetSpinnerFloat.h"

GameOptionWidgetSpinnerFloat::GameOptionWidgetSpinnerFloat(QWidget* parent, std::shared_ptr<GameOption> knownOption) : GameOptionWidget(parent), ui(new Ui::GameOptionWidgetSpinnerFloat)
{
    ui->setupUi(this);

    if (knownOption == nullptr) {
        ui->horizontalLayout->removeWidget(ui->resetButton);
        delete ui->resetButton;
    } else {
        ui->resetButton->setMaximumWidth(ui->resetButton->height());
        ui->resetButton->setToolTip(QString(tr("Default Value: %1")).arg(knownOption->getDefaultFloat()));
    }
}

GameOptionWidgetSpinnerFloat::~GameOptionWidgetSpinnerFloat()
{
    delete ui;
}

void GameOptionWidgetSpinnerFloat::setEditorData(GameOptionItem optionItem) {
    ui->doubleSpinBox->setValue(optionItem.floatValue);
}
void GameOptionWidgetSpinnerFloat::saveEditorData(GameOptionItem optionItem) {
    optionItem.floatValue = ui->doubleSpinBox->value();
}
