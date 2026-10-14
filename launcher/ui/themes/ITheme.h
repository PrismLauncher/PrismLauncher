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
#include <QFont>
#include <QList>
#include <QMap>
#include <QPalette>
#include <QString>
#include <utility>

class QStyle;

struct LogColors {
    QMap<MessageLevel, QColor> background;
    QMap<MessageLevel, QColor> foreground;
};

// Visual parameters that a palette and a style sheet cannot express.
// The defaults reproduce the classic look, so themes that don't set tokens render exactly as before.
struct ThemeTokens {
    enum class TileStyle { Classic, Slot };

    // Instance grid
    TileStyle tileStyle = TileStyle::Classic;
    int tileWidth = 100;
    int tileIconSize = 48;
    int tileSpacing = 5;
    // Show the Minecraft version and mod loader under the name, and the modpack platform on the slot
    bool tileMetaLine = false;
    // Size of one "pixel" of bevel art, in logical pixels
    int pixelUnit = 2;

    // Slot tiles: an inventory slot behind the icon and a frame around the selected one
    QColor slotFill;
    QColor slotShadow;
    QColor slotHighlight;
    QColor slotHover;
    QColor selectionFrame;

    // Text painted straight onto the instance view: group headers and tile names
    QColor viewText;
    QColor viewTextShadow;
    // Translucent box behind tile names, like a Minecraft name tag, so they stay readable over the cat
    QColor viewTextBacking;
    QString displayFont;
    int displayFontPixelSize = 0;

    // Texture tiled behind the instance view, darkened with viewTextureTint
    QString viewTexture;
    int viewTextureScale = 1;
    QColor viewTextureTint;

    // Paint the background cat in blocks of this many pixels; 0 keeps it smooth
    int catPixelSize = 0;
};

// TODO: rename to Theme; this is not an interface as it contains method implementations
// TODO: make methods const
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
    virtual ThemeTokens tokens() { return {}; }
    // Fonts for specific widget classes, such as a display face on buttons
    virtual QList<std::pair<QByteArray, QFont>> widgetFonts() { return {}; }

    static QPalette fadeInactive(QPalette in, qreal bias, QColor color);
    static LogColors defaultLogColors(const QPalette& palette);
};
