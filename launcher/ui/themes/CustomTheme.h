// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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
#pragma once

#include <QFileInfo>
#include "ITheme.h"

class CustomTheme : public ITheme {
   public:
    CustomTheme(ITheme* baseTheme, QFileInfo& file, bool isManifest);
    virtual ~CustomTheme() {}

    QString id() override;
    QString name() override;
    bool hasStyleSheet() override;
    QString appStyleSheet() override;
    bool hasColorScheme() override;
    QPalette colorScheme() override;
    double fadeAmount() override;
    QColor fadeColor() override;
    QString qtTheme() override;
    QStringList searchPaths() override;

   private: /* data */
    QPalette m_palette;
    QColor m_fadeColor;
    double m_fadeAmount;
    QString m_styleSheet;
    QString m_name;
    QString m_id;
    QString m_widgets;
    QString m_qssFilePath;
};
