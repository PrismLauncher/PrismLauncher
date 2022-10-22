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
#include "ThemeCustomizationWidget.h"
#include "ui_ThemeCustomizationWidget.h"

#include "Application.h"
#include "ui/themes/ITheme.h"

ThemeCustomizationWidget::ThemeCustomizationWidget(QWidget *parent) : QWidget(parent), ui(new Ui::ThemeCustomizationWidget)
{
    ui->setupUi(this);
    loadSettings();

    connect(ui->iconsComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ThemeCustomizationWidget::applyIconTheme);
    connect(ui->widgetStyleComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ThemeCustomizationWidget::applyWidgetTheme);
    connect(ui->backgroundCatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ThemeCustomizationWidget::applyCatTheme);
}

ThemeCustomizationWidget::~ThemeCustomizationWidget()
{
    delete ui;
}

void ThemeCustomizationWidget::showFeatures(ThemeFields features) {
    ui->iconsComboBox->setVisible(features & ThemeFields::ICONS);
    ui->iconsLabel->setVisible(features & ThemeFields::ICONS);
    ui->widgetStyleComboBox->setVisible(features & ThemeFields::WIDGETS);
    ui->widgetThemeLabel->setVisible(features & ThemeFields::WIDGETS);
    ui->backgroundCatComboBox->setVisible(features & ThemeFields::CAT);
    ui->backgroundCatLabel->setVisible(features & ThemeFields::CAT);
}

void ThemeCustomizationWidget::applyIconTheme(int index) {
    emit currentIconThemeChanged(index);

    auto settings = APPLICATION->settings();
    auto original = settings->get("IconTheme").toString();
    // FIXME: make generic
    settings->set("IconTheme", m_iconThemeOptions[index]);

    if (original != settings->get("IconTheme")) {
        APPLICATION->applyCurrentlySelectedTheme();
    }
}

void ThemeCustomizationWidget::applyWidgetTheme(int index) {
    emit currentWidgetThemeChanged(index);

    auto settings = APPLICATION->settings();
    auto originalAppTheme = settings->get("ApplicationTheme").toString();
    auto newAppTheme = ui->widgetStyleComboBox->currentData().toString();
    if (originalAppTheme != newAppTheme) {
        settings->set("ApplicationTheme", newAppTheme);
        APPLICATION->applyCurrentlySelectedTheme();
    }
}

void ThemeCustomizationWidget::applyCatTheme(int index) {
    emit currentCatChanged(index);

    auto settings = APPLICATION->settings();
    switch (index) {
        case 0: // original cat
            settings->set("BackgroundCat", "kitteh");
            break;
        case 1: // rory the cat
            settings->set("BackgroundCat", "rory");
            break;
        case 2: // rory the cat flat edition
            settings->set("BackgroundCat", "rory-flat");
            break;
        case 3:  // teawie
            settings->set("BackgroundCat", "teawie");
            break;
    }
}

void ThemeCustomizationWidget::applySettings()
{
    applyIconTheme(ui->iconsComboBox->currentIndex());
    applyWidgetTheme(ui->widgetStyleComboBox->currentIndex());
    applyCatTheme(ui->backgroundCatComboBox->currentIndex());
}
void ThemeCustomizationWidget::loadSettings()
{
    auto settings = APPLICATION->settings();

    // FIXME: make generic
    auto theme = settings->get("IconTheme").toString();
    ui->iconsComboBox->setCurrentIndex(m_iconThemeOptions.indexOf(theme));

    {
        auto currentTheme = settings->get("ApplicationTheme").toString();
        auto themes = APPLICATION->getValidApplicationThemes();
        int idx = 0;
        for (auto& theme : themes) {
            ui->widgetStyleComboBox->addItem(theme->name(), theme->id());
            if (currentTheme == theme->id()) {
                ui->widgetStyleComboBox->setCurrentIndex(idx);
            }
            idx++;
        }
    }

    auto cat = settings->get("BackgroundCat").toString();
    if (cat == "kitteh") {
        ui->backgroundCatComboBox->setCurrentIndex(0);
    } else if (cat == "rory") {
        ui->backgroundCatComboBox->setCurrentIndex(1);
    } else if (cat == "rory-flat") {
        ui->backgroundCatComboBox->setCurrentIndex(2);
    } else if (cat == "teawie") {
        ui->backgroundCatComboBox->setCurrentIndex(3);
    }
}

void ThemeCustomizationWidget::retranslate()
{
    ui->retranslateUi(this);
}