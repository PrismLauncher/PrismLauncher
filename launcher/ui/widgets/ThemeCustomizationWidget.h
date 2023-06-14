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
    ~ThemeCustomizationWidget();

    void showFeatures(ThemeFields features);

    void applySettings();

    void loadSettings();
    void retranslate();

   private slots:
    void applyIconTheme(int index);
    void applyWidgetTheme(int index);
    void applyCatTheme(int index);

   signals:
    int currentIconThemeChanged(int index);
    int currentWidgetThemeChanged(int index);
    int currentCatChanged(int index);

   private:
    Ui::ThemeCustomizationWidget* ui;

    //TODO finish implementing
    QList<std::pair<QString, QString>> m_iconThemeOptions{ 
        { "pe_colored",     QObject::tr("Simple (Colored Icons)") }, 
        { "pe_light",       QObject::tr("Simple (Light Icons)") },     
        { "pe_dark",        QObject::tr("Simple (Dark Icons)") },
        { "pe_blue",        QObject::tr("Simple (Blue Icons)") },    
        { "breeze_light",   QObject::tr("Breeze Light") }, 
        { "breeze_dark",    QObject::tr("Breeze Dark") },
        { "OSX",            QObject::tr("OSX") },        
        { "iOS",            QObject::tr("iOS") },          
        { "flat",           QObject::tr("Flat") },
        { "flat_white",     QObject::tr("Flat (White)") }, 
        { "multimc",        QObject::tr("Legacy") },      
        { "custom",         QObject::tr("Custom") } 
    };
    QList<std::pair<QString, QString>> m_catOptions{ 
        { "kitteh",     QObject::tr("Background Cat (from MultiMC)") }, 
        { "rory",       QObject::tr("Rory ID 11 (drawn by Ashtaka)") }, 
        { "rory-flat",  QObject::tr("Rory ID 11 (flat edition, drawn by Ashtaka)") },
        { "teawie",     QObject::tr("Teawie (drawn by SympathyTea)") }
    };
};
