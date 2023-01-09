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
#include "ui/themes/ThemeManager.h"

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

/// <summary>
/// The layout was not quite right, so currently this just disables the UI elements, which should be hidden instead
/// TODO FIXME
/// 
/// Original Method One:
/// ui->iconsComboBox->setVisible(features& ThemeFields::ICONS);
/// ui->iconsLabel->setVisible(features& ThemeFields::ICONS);
/// ui->widgetStyleComboBox->setVisible(features& ThemeFields::WIDGETS);
/// ui->widgetThemeLabel->setVisible(features& ThemeFields::WIDGETS);
/// ui->backgroundCatComboBox->setVisible(features& ThemeFields::CAT);
/// ui->backgroundCatLabel->setVisible(features& ThemeFields::CAT);
/// 
/// original Method Two:
///     if (!(features & ThemeFields::ICONS)) {
///         ui->formLayout->setRowVisible(0, false);
///     }
///     if (!(features & ThemeFields::WIDGETS)) {
///         ui->formLayout->setRowVisible(1, false);
///     }
///     if (!(features & ThemeFields::CAT)) {
///         ui->formLayout->setRowVisible(2, false);
///     }
/// </summary>
/// <param name="features"></param>
void ThemeCustomizationWidget::showFeatures(ThemeFields features) {
    ui->iconsComboBox->setEnabled(features & ThemeFields::ICONS);
    ui->iconsLabel->setEnabled(features & ThemeFields::ICONS);
    ui->widgetStyleComboBox->setEnabled(features & ThemeFields::WIDGETS);
    ui->widgetThemeLabel->setEnabled(features & ThemeFields::WIDGETS);
    ui->backgroundCatComboBox->setEnabled(features & ThemeFields::CAT);
    ui->backgroundCatLabel->setEnabled(features & ThemeFields::CAT);
}

void ThemeCustomizationWidget::applyIconTheme(int index) {
    auto settings = APPLICATION->settings();
    auto originalIconTheme = settings->get("IconTheme").toString();
    auto& newIconTheme = m_iconThemeOptions[index].first;
    settings->set("IconTheme", newIconTheme);

    if (originalIconTheme != newIconTheme) {
        APPLICATION->applyCurrentlySelectedTheme();
    }

    emit currentIconThemeChanged(index);
}

void ThemeCustomizationWidget::applyWidgetTheme(int index) {
    auto settings = APPLICATION->settings();
    auto originalAppTheme = settings->get("ApplicationTheme").toString();
    auto newAppTheme = ui->widgetStyleComboBox->currentData().toString();
    if (originalAppTheme != newAppTheme) {
        settings->set("ApplicationTheme", newAppTheme);
        APPLICATION->applyCurrentlySelectedTheme();
    }

    emit currentWidgetThemeChanged(index);
}

void ThemeCustomizationWidget::applyCatTheme(int index) {
    auto settings = APPLICATION->settings();
    settings->set("BackgroundCat", m_catOptions[index].first);

    emit currentCatChanged(index);
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

    auto iconTheme = settings->get("IconTheme").toString();
    for (auto& iconThemeFromList : m_iconThemeOptions) {
        QIcon iconForComboBox = QIcon(QString(":/icons/%1/scalable/settings").arg(iconThemeFromList.first));
        ui->iconsComboBox->addItem(iconForComboBox, iconThemeFromList.second);
        if (iconTheme == iconThemeFromList.first) {
            ui->iconsComboBox->setCurrentIndex(ui->iconsComboBox->count() - 1);
        }
    }

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
    for (auto& catFromList : m_catOptions) {
        QIcon catIcon = QIcon(QString(":/backgrounds/%1").arg(ThemeManager::getCatImage(catFromList.first)));
        ui->backgroundCatComboBox->addItem(catIcon, catFromList.second);
        if (cat == catFromList.first) {
            ui->backgroundCatComboBox->setCurrentIndex(ui->backgroundCatComboBox->count() - 1);
        }
    }
}

void ThemeCustomizationWidget::retranslate()
{
    ui->retranslateUi(this);
}
