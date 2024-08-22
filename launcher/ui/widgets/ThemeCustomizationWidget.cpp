// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2024 Tayou <git@tayou.org>
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
#include "DesktopServices.h"
#include "ui/themes/ITheme.h"
#include "ui/themes/ThemeManager.h"

ThemeCustomizationWidget::ThemeCustomizationWidget(QWidget* parent) : QWidget(parent), ui(new Ui::ThemeCustomizationWidget)
{
    ui->setupUi(this);
    loadSettings();
    ThemeCustomizationWidget::refresh();

    connect(ui->iconsComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ThemeCustomizationWidget::applyIconTheme);
    connect(ui->widgetStyleComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ThemeCustomizationWidget::applyWidgetTheme);
    connect(ui->backgroundCatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ThemeCustomizationWidget::applyCatTheme);

    connect(ui->iconsFolder, &QPushButton::clicked, this,
            [] { DesktopServices::openPath(APPLICATION->themeManager()->getIconThemesFolder().path()); });
    connect(ui->widgetStyleFolder, &QPushButton::clicked, this,
            [] { DesktopServices::openPath(APPLICATION->themeManager()->getApplicationThemesFolder().path()); });
    connect(ui->catPackFolder, &QPushButton::clicked, this,
            [] { DesktopServices::openPath(APPLICATION->themeManager()->getCatPacksFolder().path()); });

    connect(ui->refreshButton, &QPushButton::clicked, this, &ThemeCustomizationWidget::refresh);
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
void ThemeCustomizationWidget::showFeatures(ThemeFields features)
{
    ui->iconsComboBox->setEnabled(features & ThemeFields::ICONS);
    ui->iconsLabel->setEnabled(features & ThemeFields::ICONS);
    ui->widgetStyleComboBox->setEnabled(features & ThemeFields::WIDGETS);
    ui->widgetStyleLabel->setEnabled(features & ThemeFields::WIDGETS);
    ui->backgroundCatComboBox->setEnabled(features & ThemeFields::CAT);
    ui->backgroundCatLabel->setEnabled(features & ThemeFields::CAT);
}

void ThemeCustomizationWidget::applyIconTheme(int index)
{
    auto settings = APPLICATION->settings();
    auto originalIconTheme = settings->get("IconTheme").toString();
    auto newIconTheme = ui->iconsComboBox->currentData().toString();
    if (originalIconTheme != newIconTheme) {
        settings->set("IconTheme", newIconTheme);
        APPLICATION->themeManager()->applyCurrentlySelectedTheme();
    }

    emit currentIconThemeChanged(index);
}

void ThemeCustomizationWidget::applyWidgetTheme(int index)
{
    auto settings = APPLICATION->settings();
    auto originalAppTheme = settings->get("ApplicationTheme").toString();
    auto newAppTheme = ui->widgetStyleComboBox->currentData().toString();
    if (originalAppTheme != newAppTheme) {
        settings->set("ApplicationTheme", newAppTheme);
        APPLICATION->themeManager()->applyCurrentlySelectedTheme();
    }

    emit currentWidgetThemeChanged(index);
}

void ThemeCustomizationWidget::applyCatTheme(int index)
{
    auto settings = APPLICATION->settings();
    auto originalCat = settings->get("BackgroundCat").toString();
    auto newCat = ui->backgroundCatComboBox->currentData().toString();
    if (originalCat != newCat) {
        settings->set("BackgroundCat", newCat);
    }

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

    {
        auto currentIconTheme = settings->get("IconTheme").toString();
        auto iconThemes = APPLICATION->themeManager()->getValidIconThemes();
        int idx = 0;
        for (auto iconTheme : iconThemes) {
            QIcon iconForComboBox = QIcon(iconTheme->path() + "/scalable/settings");
            ui->iconsComboBox->addItem(iconForComboBox, iconTheme->name(), iconTheme->id());
            if (currentIconTheme == iconTheme->id()) {
                ui->iconsComboBox->setCurrentIndex(idx);
            }
            idx++;
        }
    }

    {
        auto currentTheme = settings->get("ApplicationTheme").toString();
        auto themes = APPLICATION->themeManager()->getValidApplicationThemes();
        int idx = 0;
        for (auto& theme : themes) {
            ui->widgetStyleComboBox->addItem(theme->name(), theme->id());
            if (theme->tooltip() != "") {
                int index = ui->widgetStyleComboBox->count() - 1;
                ui->widgetStyleComboBox->setItemData(index, theme->tooltip(), Qt::ToolTipRole);
            }
            if (currentTheme == theme->id()) {
                ui->widgetStyleComboBox->setCurrentIndex(idx);
            }
            idx++;
        }
    }

    auto cat = settings->get("BackgroundCat").toString();
    for (auto& catFromList : APPLICATION->themeManager()->getValidCatPacks()) {
        QIcon catIcon = QIcon(QString("%1").arg(catFromList->path()));
        ui->backgroundCatComboBox->addItem(catIcon, catFromList->name(), catFromList->id());
        if (cat == catFromList->id()) {
            ui->backgroundCatComboBox->setCurrentIndex(ui->backgroundCatComboBox->count() - 1);
        }
    }
}

void ThemeCustomizationWidget::retranslate()
{
    ui->retranslateUi(this);
}

void ThemeCustomizationWidget::refresh()
{
    applySettings();
    disconnect(ui->iconsComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ThemeCustomizationWidget::applyIconTheme);
    disconnect(ui->widgetStyleComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
               &ThemeCustomizationWidget::applyWidgetTheme);
    disconnect(ui->backgroundCatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
               &ThemeCustomizationWidget::applyCatTheme);
    APPLICATION->themeManager()->refresh();
    ui->iconsComboBox->clear();
    ui->widgetStyleComboBox->clear();
    ui->backgroundCatComboBox->clear();
    loadSettings();
    connect(ui->iconsComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ThemeCustomizationWidget::applyIconTheme);
    connect(ui->widgetStyleComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ThemeCustomizationWidget::applyWidgetTheme);
    connect(ui->backgroundCatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ThemeCustomizationWidget::applyCatTheme);
};