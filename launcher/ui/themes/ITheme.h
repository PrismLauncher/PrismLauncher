// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Tayou <git@tayou.org>
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
#pragma once
#include <MessageLevel.h>
#include <QMap>
#include <QPalette>
#include <QString>

class QStyle;

struct LogColors {
    QMap<MessageLevel::Enum, QColor> background;
    QMap<MessageLevel::Enum, QColor> foreground;
};

// TODO: rename to Theme; this is not an interface as it contains method implementations
class ITheme {
   public:
    virtual ~ITheme() {}
    virtual void apply(bool initial);
    virtual QString id() = 0;
    virtual QString name() = 0;
    virtual QString tooltip() = 0;
    virtual bool hasStyleSheet() = 0;
    virtual QString appStyleSheet() = 0;
    virtual QString qtTheme() = 0;
    virtual QPalette colorScheme() = 0;
    virtual QColor fadeColor() = 0;
    virtual double fadeAmount() = 0;
    virtual LogColors logColorScheme() { return defaultLogColors(colorScheme()); }
    virtual QStringList searchPaths() { return {}; }

    static QPalette fadeInactive(QPalette in, qreal bias, QColor color);
    static LogColors defaultLogColors(const QPalette& palette);
};
