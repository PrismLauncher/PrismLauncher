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
#include "ITheme.h"
#include "rainbow.h"
#include <QStyleFactory>
#include <QDir>
#include "Application.h"

void ITheme::apply(bool)
{
    APPLICATION->setStyleSheet(QString());
    QApplication::setStyle(QStyleFactory::create(qtTheme()));
    if (hasColorScheme()) {
        QApplication::setPalette(colorScheme());
    }
    APPLICATION->setStyleSheet(appStyleSheet());
    QDir::setSearchPaths("theme", searchPaths());
}

QPalette ITheme::fadeInactive(QPalette in, qreal bias, QColor color)
{
    auto blend = [&in, bias, color](QPalette::ColorRole role)
    {
        QColor from = in.color(QPalette::Active, role);
        QColor blended = Rainbow::mix(from, color, bias);
        in.setColor(QPalette::Disabled, role, blended);
    };
    blend(QPalette::Window);
    blend(QPalette::WindowText);
    blend(QPalette::Base);
    blend(QPalette::AlternateBase);
    blend(QPalette::ToolTipBase);
    blend(QPalette::ToolTipText);
    blend(QPalette::Text);
    blend(QPalette::Button);
    blend(QPalette::ButtonText);
    blend(QPalette::BrightText);
    blend(QPalette::Link);
    blend(QPalette::Highlight);
    blend(QPalette::HighlightedText);
    return in;
}
