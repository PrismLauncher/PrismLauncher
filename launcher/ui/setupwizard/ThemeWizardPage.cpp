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

ThemeWizardPage::~ThemeWizardPage()
{
    delete ui;
}

void ThemeWizardPage::updateIcons()
{
    qDebug() << "Setting Icons";
    ui->previewIconButton0->setIcon(APPLICATION->getThemedIcon("new"));
    ui->previewIconButton1->setIcon(APPLICATION->getThemedIcon("centralmods"));
    ui->previewIconButton2->setIcon(APPLICATION->getThemedIcon("viewfolder"));
    ui->previewIconButton3->setIcon(APPLICATION->getThemedIcon("launch"));
    ui->previewIconButton4->setIcon(APPLICATION->getThemedIcon("copy"));
    ui->previewIconButton5->setIcon(APPLICATION->getThemedIcon("export"));
    ui->previewIconButton6->setIcon(APPLICATION->getThemedIcon("delete"));
    ui->previewIconButton7->setIcon(APPLICATION->getThemedIcon("about"));
    ui->previewIconButton8->setIcon(APPLICATION->getThemedIcon("settings"));
    ui->previewIconButton9->setIcon(APPLICATION->getThemedIcon("cat"));
    update();
    repaint();
    parentWidget()->update();
}

void ThemeWizardPage::updateCat()
{
    qDebug() << "Setting Cat";
    ui->catImagePreviewButton->setIcon(QIcon(QString(R"(%1)").arg(APPLICATION->themeManager()->getCatPack())));
}

void ThemeWizardPage::retranslate()
{
    ui->retranslateUi(this);
}
