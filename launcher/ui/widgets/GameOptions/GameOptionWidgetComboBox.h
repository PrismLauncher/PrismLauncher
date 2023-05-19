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
#pragma once

#include <QString>
#include <QWidget>

#include "GameOptionWidget.h"

namespace Ui {
class GameOptionWidgetComboBox;
}

class GameOptionWidgetComboBox : public GameOptionWidget {

   public:
    explicit GameOptionWidgetComboBox(QWidget* parent, std::shared_ptr<GameOption> knownOption);
    ~GameOptionWidgetComboBox() override;
    void setEditorData(GameOptionItem optionItem) override;
    void saveEditorData(GameOptionItem optionItem) override;

   private:
    Ui::GameOptionWidgetComboBox* ui;
};
