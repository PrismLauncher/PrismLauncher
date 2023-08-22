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
#include "BaseWizardPage.h"

namespace Ui {
class ThemeWizardPage;
}

class ThemeWizardPage : public BaseWizardPage {
    Q_OBJECT

   public:
    explicit ThemeWizardPage(QWidget* parent = nullptr);
    ~ThemeWizardPage();

    bool validatePage() override { return true; };
    void retranslate() override;

   private slots:
    void updateIcons();
    void updateCat();

   private:
    Ui::ThemeWizardPage* ui;
};
