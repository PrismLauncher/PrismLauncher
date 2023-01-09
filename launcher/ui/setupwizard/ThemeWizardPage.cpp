// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Tayou <tayou@gmx.net>
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
#include "ui/widgets/ThemeCustomizationWidget.h"
#include "ui_ThemeCustomizationWidget.h"

ThemeWizardPage::ThemeWizardPage(QWidget* parent) : BaseWizardPage(parent), ui(new Ui::ThemeWizardPage)
{
    ui->setupUi(this);

    connect(ui->themeCustomizationWidget, QOverload<int>::of(&ThemeCustomizationWidget::currentIconThemeChanged), this, &ThemeWizardPage::updateIcons);
    connect(ui->themeCustomizationWidget, QOverload<int>::of(&ThemeCustomizationWidget::currentCatChanged), this, &ThemeWizardPage::updateCat);

    updateIcons();
    updateCat();
}

ThemeWizardPage::~ThemeWizardPage()
{
    delete ui;
}

void ThemeWizardPage::initializePage() {}

bool ThemeWizardPage::validatePage()
{
    return true;
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

    QDateTime now = QDateTime::currentDateTime();
    QDateTime birthday(QDate(now.date().year(), 11, 30), QTime(0, 0));
    QDateTime xmas(QDate(now.date().year(), 12, 25), QTime(0, 0));
    QDateTime halloween(QDate(now.date().year(), 10, 31), QTime(0, 0));
    QString cat = APPLICATION->settings()->get("BackgroundCat").toString();
    if (std::abs(now.daysTo(xmas)) <= 4) {
        cat += "-xmas";
    } else if (std::abs(now.daysTo(halloween)) <= 4) {
        cat += "-spooky";
    } else if (std::abs(now.daysTo(birthday)) <= 12) {
        cat += "-bday";
    }
    ui->catImagePreviewButton->setIcon(QIcon(QString(R"(:/backgrounds/%1)").arg(cat)));
}

void ThemeWizardPage::retranslate()
{
    ui->retranslateUi(this);
}
