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

#include "GameOptionWidgetComboBox.h"
#include "ui_GameOptionWidgetComboBox.h"

GameOptionWidgetComboBox::GameOptionWidgetComboBox(QWidget* parent, std::shared_ptr<GameOption> knownOption) : GameOptionWidget(parent), ui(new Ui::GameOptionWidgetComboBox)
{
    ui->setupUi(this);

    ui->resetButton->setMaximumWidth(ui->resetButton->height());
    ui->resetButton->setToolTip(QString(tr("Default Value: %1")).arg(knownOption->getDefaultString()));

    for (const auto& value : knownOption->validValues) {
        ui->comboBox->addItem(value);
    }
}

GameOptionWidgetComboBox::~GameOptionWidgetComboBox()
{
    delete ui;
}
void GameOptionWidgetComboBox::setEditorData(GameOptionItem optionItem) {
    ui->comboBox->setCurrentIndex(ui->comboBox->findText(optionItem.value));
}
void GameOptionWidgetComboBox::saveEditorData(GameOptionItem optionItem) {
    optionItem.value = ui->comboBox->currentText();
}
