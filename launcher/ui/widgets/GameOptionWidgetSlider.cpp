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

#include "GameOptionWidgetSlider.h"
#include "Application.h"
#include "minecraft/gameoptions/GameOptions.h"
#include "ui_GameOptionWidgetSlider.h"

#include <QString>
#include <QStyledItemDelegate>
#include <QWidget>

GameOptionWidgetSlider::GameOptionWidgetSlider(QWidget* parent, std::shared_ptr<GameOption> knownOption)
    : GameOptionWidget(parent), ui(new Ui::GameOptionWidgetSlider)
{
    ui->setupUi(this);

    ui->resetButton->setMaximumWidth(ui->resetButton->height());

    if (knownOption->type == OptionType::Float) {
        ui->resetButton->setToolTip(QString(tr("Default Value: %1")).arg(knownOption->getDefaultFloat()));

        ui->slider->setMinimum(knownOption->getFloatRange().min * 100);
        ui->slider->setMaximum(knownOption->getFloatRange().max * 100);

        // QLocale((QString)APPLICATION->settings()->get("Language"))
        ui->maxLabel->setText(QString("%1").arg(knownOption->getFloatRange().max));
        ui->minLabel->setText(QString("%1").arg(knownOption->getFloatRange().min));

        connect(ui->slider, QOverload<int>::of(&QSlider::valueChanged), this, [&](int value) { ui->spinBox->setValue(value); });
        connect(ui->spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [&](int value) { ui->slider->setValue(value); });
    } else { // Int slider
        ui->resetButton->setToolTip(QString(tr("Default Value: %1")).arg(knownOption->getDefaultInt()));

        ui->slider->setMinimum(knownOption->getIntRange().min);
        ui->slider->setMaximum(knownOption->getIntRange().max);

        ui->maxLabel->setText(QString("%1").arg(knownOption->getIntRange().max));
        ui->minLabel->setText(QString("%1").arg(knownOption->getIntRange().min));

        connect(ui->slider, QOverload<int>::of(&QSlider::valueChanged), this, [&](int value) { ui->spinBox->setValue(value / 100.0f); });
        connect(ui->spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [&](int value) { ui->slider->setValue(value * 100.0f); });
    }
}

void GameOptionWidgetSlider::setEditorData(GameOptionItem optionItem) {
    switch (optionItem.type) {
        case OptionType::Int: {
            ui->slider->setValue(optionItem.intValue);
            break;
        }
        case OptionType::Float: {
            ui->slider->setValue(optionItem.floatValue * 100);
            break;
        }
        default:
            break;
    }
}
GameOptionWidgetSlider::~GameOptionWidgetSlider() {
    destroy(ui);
}
void GameOptionWidgetSlider::saveEditorData(GameOptionItem optionItem) {
    switch (optionItem.type) {
        case OptionType::Int: {
            optionItem.intValue = ui->slider->value();
            break;
        }
        case OptionType::Float: {
            optionItem.floatValue = ui->slider->value() / 100.0f;
            break;
        }
        default:
            break;
    }
}
