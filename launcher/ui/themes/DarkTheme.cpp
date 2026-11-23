// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2024 Tayou <git@tayou.org>
 *  Copyright (C) 2024 TheKodeToad <TheKodeToad@proton.me>
 *  Copyright (C) 2026 JAGUARX CLIENT contributors
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
#include "DarkTheme.h"

#include <QObject>

QString DarkTheme::id()
{
    // Keep the existing ID so saved user preferences continue to select this theme.
    return "dark";
}

QString DarkTheme::name()
{
    return QObject::tr("JAGUARX Dark");
}

QPalette DarkTheme::colorScheme()
{
    // A restrained blue-on-charcoal palette for JAGUARX CLIENT.
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(25, 30, 40));
    darkPalette.setColor(QPalette::WindowText, QColor(242, 245, 250));
    darkPalette.setColor(QPalette::Base, QColor(17, 21, 29));
    darkPalette.setColor(QPalette::AlternateBase, QColor(30, 37, 49));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(35, 105, 210));
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, QColor(235, 239, 247));
    darkPalette.setColor(QPalette::Button, QColor(34, 41, 54));
    darkPalette.setColor(QPalette::ButtonText, QColor(242, 245, 250));
    darkPalette.setColor(QPalette::BrightText, QColor(255, 96, 96));
    darkPalette.setColor(QPalette::Link, QColor(92, 163, 255));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 119, 235));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);
    darkPalette.setColor(QPalette::PlaceholderText, QColor(145, 155, 172));
    return fadeInactive(darkPalette, fadeAmount(), fadeColor());
}

double DarkTheme::fadeAmount()
{
    return 0.5;
}

QColor DarkTheme::fadeColor()
{
    return QColor(25, 30, 40);
}

bool DarkTheme::hasStyleSheet()
{
    return false;
}

QString DarkTheme::appStyleSheet()
{
    return "";
}

QString DarkTheme::tooltip()
{
    return "";
}
