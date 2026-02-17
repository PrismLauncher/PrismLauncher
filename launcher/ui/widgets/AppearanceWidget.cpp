// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2025 TheKodeToad <TheKodeToad@proton.me>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "AppearanceWidget.h"
#include "ui_AppearanceWidget.h"

#include <DesktopServices.h>
#include <QGraphicsOpacityEffect>
#include "BuildConfig.h"
#include "ui/themes/ITheme.h"
#include "ui/themes/ThemeManager.h"

AppearanceWidget::AppearanceWidget(bool themesOnly, QWidget* parent)
    : QWidget(parent), m_ui(new Ui::AppearanceWidget), m_themesOnly(themesOnly)
{
    m_ui->setupUi(this);

    m_ui->catPreview->setGraphicsEffect(new QGraphicsOpacityEffect(this));

    m_defaultFormat = QTextCharFormat(m_ui->consolePreview->currentCharFormat());

    if (themesOnly) {
        m_ui->catPackLabel->hide();
        m_ui->catPackComboBox->hide();
        m_ui->catPackFolder->hide();
        m_ui->settingsBox->hide();
        m_ui->consolePreview->hide();
        m_ui->catPreview->hide();
        loadThemeSettings();
    } else {
        loadSettings();
        loadThemeSettings();

        updateConsolePreview();
        updateCatPreview();
    }

    connect(m_ui->fontSizeBox, &QSpinBox::valueChanged, this, &AppearanceWidget::updateConsolePreview);
    connect(m_ui->consoleFont, &QFontComboBox::currentFontChanged, this, &AppearanceWidget::updateConsolePreview);

    connect(m_ui->iconsComboBox, &QComboBox::currentIndexChanged, this, &AppearanceWidget::applyIconTheme);
    connect(m_ui->widgetStyleComboBox, &QComboBox::currentIndexChanged, this, &AppearanceWidget::applyWidgetTheme);
    connect(m_ui->catPackComboBox, &QComboBox::currentIndexChanged, this, &AppearanceWidget::applyCatTheme);
    connect(m_ui->catOpacitySlider, &QAbstractSlider::valueChanged, this, &AppearanceWidget::updateCatPreview);

    connect(m_ui->iconsFolder, &QPushButton::clicked, this,
            [] { DesktopServices::openPath(APPLICATION->themeManager()->getIconThemesFolder().path()); });
    connect(m_ui->widgetStyleFolder, &QPushButton::clicked, this,
            [] { DesktopServices::openPath(APPLICATION->themeManager()->getApplicationThemesFolder().path()); });
    connect(m_ui->catPackFolder, &QPushButton::clicked, this,
            [] { DesktopServices::openPath(APPLICATION->themeManager()->getCatPacksFolder().path()); });
    connect(m_ui->reloadThemesButton, &QPushButton::pressed, this, &AppearanceWidget::loadThemeSettings);
}

AppearanceWidget::~AppearanceWidget()
{
    delete m_ui;
}

void AppearanceWidget::applySettings()
{
    SettingsObject* settings = APPLICATION->settings();
    QString consoleFontFamily = m_ui->consoleFont->currentFont().family();
    settings->set("ConsoleFont", consoleFontFamily);
    settings->set("ConsoleFontSize", m_ui->fontSizeBox->value());
    settings->set("CatOpacity", m_ui->catOpacitySlider->value());
    auto catFit = m_ui->catFitComboBox->currentIndex();
    settings->set("CatFit", catFit == 0 ? "fit" : catFit == 1 ? "fill" : "strech");
}

void AppearanceWidget::loadSettings()
{
    SettingsObject* settings = APPLICATION->settings();
    QString fontFamily = settings->get("ConsoleFont").toString();
    QFont consoleFont(fontFamily);
    m_ui->consoleFont->setCurrentFont(consoleFont);

    bool conversionOk = true;
    int fontSize = settings->get("ConsoleFontSize").toInt(&conversionOk);
    if (!conversionOk) {
        fontSize = 11;
    }
    m_ui->fontSizeBox->setValue(fontSize);

    m_ui->catOpacitySlider->setValue(settings->get("CatOpacity").toInt());

    auto catFit = settings->get("CatFit").toString();
    m_ui->catFitComboBox->setCurrentIndex(catFit == "fit" ? 0 : catFit == "fill" ? 1 : 2);
}

void AppearanceWidget::retranslateUi()
{
    m_ui->retranslateUi(this);
}

void AppearanceWidget::applyIconTheme(int index)
{
    auto settings = APPLICATION->settings();
    auto originalIconTheme = settings->get("IconTheme").toString();
    auto newIconTheme = m_ui->iconsComboBox->itemData(index).toString();
    if (originalIconTheme != newIconTheme) {
        settings->set("IconTheme", newIconTheme);
        APPLICATION->themeManager()->applyCurrentlySelectedTheme();
    }
}

void AppearanceWidget::applyWidgetTheme(int index)
{
    auto settings = APPLICATION->settings();
    auto originalAppTheme = settings->get("ApplicationTheme").toString();
    auto newAppTheme = m_ui->widgetStyleComboBox->itemData(index).toString();
    if (originalAppTheme != newAppTheme) {
        settings->set("ApplicationTheme", newAppTheme);
        APPLICATION->themeManager()->applyCurrentlySelectedTheme();
    }

    updateConsolePreview();
}

void AppearanceWidget::applyCatTheme(int index)
{
    auto settings = APPLICATION->settings();
    auto originalCat = settings->get("BackgroundCat").toString();
    auto newCat = m_ui->catPackComboBox->itemData(index).toString();
    if (originalCat != newCat) {
        settings->set("BackgroundCat", newCat);
    }

    APPLICATION->currentCatChanged(index);
    updateCatPreview();
}

void AppearanceWidget::loadThemeSettings()
{
    APPLICATION->themeManager()->refresh();

    m_ui->iconsComboBox->blockSignals(true);
    m_ui->widgetStyleComboBox->blockSignals(true);
    m_ui->catPackComboBox->blockSignals(true);

    m_ui->iconsComboBox->clear();
    m_ui->widgetStyleComboBox->clear();
    m_ui->catPackComboBox->clear();

    SettingsObject* settings = APPLICATION->settings();

    const QString currentIconTheme = settings->get("IconTheme").toString();
    const auto iconThemes = APPLICATION->themeManager()->getValidIconThemes();

    for (int i = 0; i < iconThemes.count(); ++i) {
        const IconTheme* theme = iconThemes[i];

        QIcon iconForComboBox = QIcon(theme->path() + "/scalable/settings");
        m_ui->iconsComboBox->addItem(iconForComboBox, theme->name(), theme->id());

        if (currentIconTheme == theme->id())
            m_ui->iconsComboBox->setCurrentIndex(i);
    }

    const QString currentTheme = settings->get("ApplicationTheme").toString();
    auto themes = APPLICATION->themeManager()->getValidApplicationThemes();
    for (int i = 0; i < themes.count(); ++i) {
        ITheme* theme = themes[i];

        m_ui->widgetStyleComboBox->addItem(theme->name(), theme->id());

        if (!theme->tooltip().isEmpty())
            m_ui->widgetStyleComboBox->setItemData(i, theme->tooltip(), Qt::ToolTipRole);

        if (currentTheme == theme->id())
            m_ui->widgetStyleComboBox->setCurrentIndex(i);
    }

    if (!m_themesOnly) {
        const QString currentCat = settings->get("BackgroundCat").toString();
        const auto cats = APPLICATION->themeManager()->getValidCatPacks();
        for (int i = 0; i < cats.count(); ++i) {
            const CatPack* cat = cats[i];

            QIcon catIcon = QIcon(QString("%1").arg(cat->path()));
            m_ui->catPackComboBox->addItem(catIcon, cat->name(), cat->id());

            if (currentCat == cat->id())
                m_ui->catPackComboBox->setCurrentIndex(i);
        }
    }

    m_ui->iconsComboBox->blockSignals(false);
    m_ui->widgetStyleComboBox->blockSignals(false);
    m_ui->catPackComboBox->blockSignals(false);
}

void AppearanceWidget::updateConsolePreview()
{
    const LogColors& colors = APPLICATION->themeManager()->getLogColors();

    int fontSize = m_ui->fontSizeBox->value();
    QString fontFamily = m_ui->consoleFont->currentFont().family();
    m_ui->consolePreview->clear();
    m_defaultFormat.setFont(QFont(fontFamily, fontSize));

    auto print = [this, colors](const QString& message, MessageLevel level) {
        QTextCharFormat format(m_defaultFormat);

        QColor bg = colors.background.value(level);
        QColor fg = colors.foreground.value(level);

        if (bg.isValid())
            format.setBackground(bg);

        if (fg.isValid())
            format.setForeground(fg);

        // append a paragraph/line
        auto workCursor = m_ui->consolePreview->textCursor();
        workCursor.movePosition(QTextCursor::End);
        workCursor.insertText(message, format);
        workCursor.insertBlock();
    };

    print(QString("%1 version: %2\n").arg(BuildConfig.LAUNCHER_DISPLAYNAME, BuildConfig.printableVersionString()), MessageLevel::Launcher);

    QDate today = QDate::currentDate();

    if (today.month() == 10 && today.day() == 31)
        print(tr("[ERROR] OOoooOOOoooo! A spooky error!"), MessageLevel::Error);
    else
        print(tr("[ERROR] A spooky error!"), MessageLevel::Error);

    print(tr("[INFO] A harmless message..."), MessageLevel::Info);
    print(tr("[WARN] A not so spooky warning."), MessageLevel::Warning);
    print(tr("[DEBUG] A secret debugging message..."), MessageLevel::Debug);
    print(tr("[FATAL] A terrifying fatal error!"), MessageLevel::Fatal);
}

void AppearanceWidget::updateCatPreview()
{
    QIcon catPackIcon(APPLICATION->themeManager()->getCatPack());
    m_ui->catPreview->setIcon(catPackIcon);

    auto effect = dynamic_cast<QGraphicsOpacityEffect*>(m_ui->catPreview->graphicsEffect());
    if (effect)
        effect->setOpacity(m_ui->catOpacitySlider->value() / 100.0);
}
