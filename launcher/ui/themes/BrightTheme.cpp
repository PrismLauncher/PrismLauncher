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
#include "BrightTheme.h"

#include <QObject>

QString BrightTheme::id()
{
    return "bright";
}

QString BrightTheme::name()
{
    return QObject::tr("Bright");
}

QPalette BrightTheme::colorScheme()
{
    QPalette brightPalette;
    brightPalette.setColor(QPalette::Window, QColor(255, 255, 255));
    brightPalette.setColor(QPalette::WindowText, QColor(17, 17, 17));
    brightPalette.setColor(QPalette::Base, QColor(250, 250, 250));
    brightPalette.setColor(QPalette::AlternateBase, QColor(240, 240, 240));
    brightPalette.setColor(QPalette::ToolTipBase, QColor(17, 17, 17));
    brightPalette.setColor(QPalette::ToolTipText, QColor(255, 255, 255));
    brightPalette.setColor(QPalette::Text, Qt::black);
    brightPalette.setColor(QPalette::Button, QColor(249, 249, 249));
    brightPalette.setColor(QPalette::ButtonText, Qt::black);
    brightPalette.setColor(QPalette::BrightText, Qt::red);
    brightPalette.setColor(QPalette::Link, QColor(37, 137, 164));
    brightPalette.setColor(QPalette::Highlight, QColor(137, 207, 84));
    brightPalette.setColor(QPalette::HighlightedText, Qt::black);
    return fadeInactive(brightPalette, fadeAmount(), fadeColor());
}

double BrightTheme::fadeAmount()
{
    return 0.5;
}

QColor BrightTheme::fadeColor()
{
    return QColor(255, 255, 255);
}

bool BrightTheme::hasStyleSheet()
{
    return false;
}

QString BrightTheme::appStyleSheet()
{
    return QString();
}
QString BrightTheme::tooltip()
{
    return QString();
}
