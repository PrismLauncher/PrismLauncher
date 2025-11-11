// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2024 Tayou <git@tayou.org>
 *  Copyright (C) 2024 TheKodeToad <TheKodeToad@proton.me>
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
#include <QStyle>
#include <QStyleFactory>
#include "HintOverrideProxyStyle.h"
#include "ThemeManager.h"

// See https://github.com/MultiMC/Launcher/issues/1790
// or https://github.com/PrismLauncher/PrismLauncher/issues/490
static const QStringList S_NATIVE_STYLES{ "windows11", "windowsvista", "macos", "system", "windows" };

SystemTheme::SystemTheme(const QString& styleName, const QPalette& defaultPalette, bool isDefaultTheme)
{
    m_themeName = isDefaultTheme ? "system" : styleName;
    m_widgetTheme = styleName;
    // NOTE: SystemTheme is reconstructed on page refresh. We can't accurately determine the system palette here
    // See also S_NATIVE_STYLES comment
    if (S_NATIVE_STYLES.contains(m_themeName)) {
        m_colorPalette = defaultPalette;
    } else {
        auto style = QStyleFactory::create(styleName);
        m_colorPalette = style != nullptr ? style->standardPalette() : defaultPalette;
        delete style;
    }
}

void SystemTheme::apply(bool initial)
{
    // See S_NATIVE_STYLES comment
    if (initial && S_NATIVE_STYLES.contains(m_themeName)) {
        QApplication::setStyle(new HintOverrideProxyStyle(QStyleFactory::create(qtTheme())));
        return;
    }

    ITheme::apply(initial);
}

QString SystemTheme::id()
{
    return m_themeName;
}

QString SystemTheme::name()
{
    if (m_themeName.toLower() == "windowsvista") {
        return QObject::tr("Windows Vista");
    } else if (m_themeName.toLower() == "windows") {
        return QObject::tr("Windows 9x");
    } else if (m_themeName.toLower() == "windows11") {
        return QObject::tr("Windows 11");
    } else if (m_themeName.toLower() == "system") {
        return QObject::tr("System");
    } else {
        return m_themeName;
    }
}

QString SystemTheme::tooltip()
{
    if (m_themeName.toLower() == "windowsvista") {
        return QObject::tr("Widget style trying to look like your win32 theme");
    } else if (m_themeName.toLower() == "windows") {
        return QObject::tr("Windows 9x inspired widget style");
    } else if (m_themeName.toLower() == "windows11") {
        return QObject::tr("WinUI 3 inspired Qt widget style");
    } else if (m_themeName.toLower() == "fusion") {
        return QObject::tr("The default Qt widget style");
    } else if (m_themeName.toLower() == "system") {
        return QObject::tr("Your current system theme");
    } else {
        return "";
    }
}

QString SystemTheme::qtTheme()
{
    return m_widgetTheme;
}

QPalette SystemTheme::colorScheme()
{
    return m_colorPalette;
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
