// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher
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

#include <QString>

#include "ui/themes/ITheme.h"
#include "ui/MainWindow.h"

#define themeDebugLog qDebug() << "[Themes]"
#define themeWarningLog qWarning() << "[Themes]"

class ThemeManager {
public:
    ThemeManager(MainWindow* mainWindow);
    void InitializeThemes();

    std::vector<ITheme *> getValidApplicationThemes();
    void setIconTheme(const QString& name);
    void applyCurrentlySelectedTheme();
    void setApplicationTheme(const QString& name, bool initial);

private:
    std::map<QString, std::unique_ptr<ITheme>> m_themes;
    MainWindow* m_mainWindow;

    QString AddTheme(ITheme * theme);
    ITheme* GetTheme(QString themeId);
};

