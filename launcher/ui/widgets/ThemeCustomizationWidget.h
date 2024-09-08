// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Tayou <git@tayou.org>
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

#include <QWidget>
#include "translations/TranslationsModel.h"

enum ThemeFields { NONE = 0b0000, ICONS = 0b0001, WIDGETS = 0b0010, CAT = 0b0100 };

namespace Ui {
class ThemeCustomizationWidget;
}

class ThemeCustomizationWidget : public QWidget {
    Q_OBJECT

   public:
    explicit ThemeCustomizationWidget(QWidget* parent = nullptr);
    ~ThemeCustomizationWidget() override;

    void showFeatures(ThemeFields features);

    void applySettings();

    void loadSettings();
    void retranslate();

   private slots:
    void applyIconTheme(int index);
    void applyWidgetTheme(int index);
    void applyCatTheme(int index);
    void refresh();

   signals:
    int currentIconThemeChanged(int index);
    int currentWidgetThemeChanged(int index);
    int currentCatChanged(int index);

   private:
    Ui::ThemeCustomizationWidget* ui;
};
