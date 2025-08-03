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
#include "ThemeWizardPage.h"
#include "ui_ThemeWizardPage.h"

#include "Application.h"
#include "ui/themes/ITheme.h"
#include "ui/themes/ThemeManager.h"
#include "ui/widgets/ThemeCustomizationWidget.h"
#include "ui_ThemeCustomizationWidget.h"

ThemeWizardPage::ThemeWizardPage(QWidget* parent) : BaseWizardPage(parent), ui(new Ui::ThemeWizardPage)
{
    ui->setupUi(this);

    connect(ui->themeCustomizationWidget, &ThemeCustomizationWidget::currentIconThemeChanged, this, &ThemeWizardPage::updateIcons);
    connect(ui->themeCustomizationWidget, &ThemeCustomizationWidget::currentCatChanged, this, &ThemeWizardPage::updateCat);

    updateIcons();
    updateCat();
}
{
    ui->retranslateUi(this);
}
