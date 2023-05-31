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
 */
#pragma once

#include <QString>

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
    MainWindow* m_mainWindow;

    void initializeThemes();
    QString addTheme(std::unique_ptr<ITheme> theme);
    ITheme* getTheme(QString themeId);
};
