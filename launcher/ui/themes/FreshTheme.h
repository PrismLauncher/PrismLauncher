// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Agustin <agustinm28@gmail.com>
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

#include "FusionTheme.h"

class QWidget;

// Built-in theme for the aesthetic overhaul.
// Colors (per palette), radii and spacing come from token tables in FreshTheme.cpp; fresh.qss refers to them as @token.
class FreshTheme : public FusionTheme {
   public:
    // same theme, two color palettes: warm earth tones or cool stone grays
    enum class Palette { Terra, Stone };
    explicit FreshTheme(Palette palette);

    QString id() override;
    QString name() override;
    QString tooltip() override;
    bool hasStyleSheet() override;
    QString appStyleSheet() override;
    QPalette colorScheme() override;
    double fadeAmount() override;
    QColor fadeColor() override;
    void apply(bool initial) override;
    QStyle* createStyle() override;
    QString preferredIconTheme() override { return "pe_colored"; }
    bool tintsTitlebar() override { return true; }

    // for widgets that paint themselves (e.g. the instance grid) and only restyle under Fresh
    static bool isActive();
    static QColor color(const QString& token);
    // fades a widget in (e.g. a newly shown page); no-op unless Fresh is active
    static void fadeIn(QWidget* widget, int duration = 160);

   private:
    Palette m_palette;
};
