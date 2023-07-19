// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Tayou <git@tayou.org>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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

#include <QString>

#include "IconTheme.h"
#include "ui/MainWindow.h"
#include "ui/themes/ITheme.h"

inline auto themeDebugLog()
{
    return qDebug() << "[Theme]";
}
inline auto themeWarningLog()
{
    return qWarning() << "[Theme]";
}

class ThemeManager {
   public:
    ThemeManager(MainWindow* mainWindow);

    QList<ITheme*> getValidApplicationThemes();
    QList<IconTheme*> getValidIconThemes();
    void setIconTheme(const QString& name);
    void applyCurrentlySelectedTheme(bool initial = false);
    void setApplicationTheme(const QString& name, bool initial = false);

    /// <summary>
    /// Returns the cat based on selected cat and with events (Birthday, XMas, etc.)
    /// </summary>
    /// <param name="catName">Optional, if you need a specific cat.</param>
    /// <returns></returns>
    static QString getCatImage(QString catName = "");

   private:
    std::map<QString, std::unique_ptr<ITheme>> m_themes;
    QList<IconTheme> m_icons;
    MainWindow* m_mainWindow;

    void initializeThemes();
    QString addTheme(std::unique_ptr<ITheme> theme);
    ITheme* getTheme(QString themeId);
    void initializeIcons();
    void initializeWidgets();

    const QStringList builtinIcons{ "pe_colored", "pe_light", "pe_dark", "pe_blue",    "breeze_light", "breeze_dark",
                                    "OSX",        "iOS",      "flat",    "flat_white", "multimc" };
};
