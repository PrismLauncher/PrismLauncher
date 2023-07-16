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
#include "SystemTheme.h"
#include <QApplication>
#include <QDebug>
#include <QStyle>
#include <QStyleFactory>
#include "ThemeManager.h"

SystemTheme::SystemTheme()
{
    themeDebugLog() << "Determining System Theme...";
    const auto& style = QApplication::style();
    systemPalette = QApplication::palette();
    QString lowerThemeName = style->objectName();
    themeDebugLog() << "System theme seems to be:" << lowerThemeName;
    QStringList styles = QStyleFactory::keys();
    for (auto& st : styles) {
        themeDebugLog() << "Considering theme from theme factory:" << st.toLower();
        if (st.toLower() == lowerThemeName) {
            systemTheme = st;
            themeDebugLog() << "System theme has been determined to be:" << systemTheme;
            return;
        }
    }
    // fall back to fusion if we can't find the current theme.
    systemTheme = "Fusion";
    themeDebugLog() << "System theme not found, defaulted to Fusion";
}

void SystemTheme::apply(bool initial)
{
    // See https://github.com/MultiMC/Launcher/issues/1790
    // or https://github.com/PrismLauncher/PrismLauncher/issues/490
    if (initial)
        return; 
    ITheme::apply(initial);
}

QString SystemTheme::id()
{
    return "system";
}

QString SystemTheme::name()
{
    return QObject::tr("System");
}

QString SystemTheme::qtTheme()
{
    return systemTheme;
}

QPalette SystemTheme::colorScheme()
{
    return systemPalette;
}

QString SystemTheme::appStyleSheet()
{
    return QString();
}

double SystemTheme::fadeAmount()
{
    return 0.5;
}

QColor SystemTheme::fadeColor()
{
    return QColor(128, 128, 128);
}

bool SystemTheme::hasStyleSheet()
{
    return false;
}

bool SystemTheme::hasColorScheme()
{
    return true;
}
