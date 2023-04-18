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
#include "GameOptionDelegate.h"
#include "ui/widgets/GameOptionWidget.h"
#include "ui/widgets/GameOptionWidgetCheckBox.h"
#include "ui/widgets/GameOptionWidgetComboBox.h"
#include "ui/widgets/GameOptionWidgetKeyBind.h"
#include "ui/widgets/GameOptionWidgetSlider.h"
#include "ui/widgets/GameOptionWidgetSpinnerFloat.h"
#include "ui/widgets/GameOptionWidgetSpinnerInt.h"
#include "ui/widgets/GameOptionWidgetText.h"

#include <QDebug>

QWidget* GameOptionDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    std::shared_ptr<GameOption> knownOption = contents->at(index.row()).knownOption;

    switch (contents->at(index.row()).type) {
        case OptionType::String: {
            if (knownOption != nullptr && !knownOption->validValues.isEmpty()) {
                return new GameOptionWidgetComboBox(parent, knownOption);
            } else {
                return new GameOptionWidgetText(parent, knownOption);
            }
        }
        case OptionType::Int: {
            if (knownOption != nullptr && (knownOption->getIntRange().max != 0 || knownOption->getIntRange().min != 0)) {
                return new GameOptionWidgetSlider(parent, knownOption);
            } else {
                return new GameOptionWidgetSpinnerInt(parent, knownOption);
            }
        }
        case OptionType::Bool: {
            return new GameOptionWidgetCheckBox(parent, knownOption);
        }
        case OptionType::Float: {
            if (knownOption != nullptr && (knownOption->getFloatRange().max != 0 || knownOption->getFloatRange().min != 0)) {
                return new GameOptionWidgetSlider(parent, knownOption);
            } else {
                return new GameOptionWidgetSpinnerFloat(parent, knownOption);
            }
        }
        case OptionType::KeyBind: {
            return new GameOptionWidgetKeyBind(parent, knownOption);
        }
        default:
            return nullptr;
    }
}

void GameOptionDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    GameOptionWidget* widget = dynamic_cast<GameOptionWidget*>(editor);

    GameOptionItem optionItem = contents->at(index.row());

    if (widget != nullptr) {
        widget->setEditorData(optionItem);
    } else {
        qDebug() << "[GameOptions] Setting widget data failed because widget was still null";
    }
};

void GameOptionDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    GameOptionWidget* widget = dynamic_cast<GameOptionWidget*>(editor);
    widget->setSize(option.rect);
};

void GameOptionDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    GameOptionWidget* widget = dynamic_cast<GameOptionWidget*>(editor);

    GameOptionItem optionItem = contents->at(index.row());

    if (widget != nullptr) {
        widget->saveEditorData(optionItem);
    } else {
        qDebug() << "[GameOptions] Saving widget data to Model failed because widget was null";
    }
}