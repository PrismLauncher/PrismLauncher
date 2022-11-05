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
#include "ThemeManager.h"

#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QIcon>
#include "ui/themes/BrightTheme.h"
#include "ui/themes/CustomTheme.h"
#include "ui/themes/DarkTheme.h"
#include "ui/themes/SystemTheme.h"

#include "Application.h"

#ifdef Q_OS_WIN
#include <windows.h>
// this is needed for versionhelpers.h, it is also included in WinDarkmode, but we can't rely on that.
// Ultimately this should be included in versionhelpers, but that is outside of the project.
#include "ui/WinDarkmode.h"
#include <versionhelpers.h>
#endif

ThemeManager::ThemeManager(MainWindow* mainWindow)
{
    m_mainWindow = mainWindow;
    InitializeThemes();
}

/// @brief Adds the Theme to the list of themes
/// @param theme The Theme to add
/// @return Theme ID
QString ThemeManager::AddTheme(std::unique_ptr<ITheme> theme)
{
    QString id = theme->id();
    m_themes.emplace(id, std::move(theme));
    return id;
}

/// @brief Gets the Theme from the List via ID
/// @param themeId Theme ID of theme to fetch
/// @return Theme at themeId
ITheme* ThemeManager::GetTheme(QString themeId)
{
    return m_themes[themeId].get();
}

void ThemeManager::InitializeThemes()
{
    // Icon themes
    {
        // TODO: icon themes and instance icons do not mesh well together. Rearrange and fix discrepancies!
        // set icon theme search path!
        auto searchPaths = QIcon::themeSearchPaths();
        searchPaths.append("iconthemes");
        QIcon::setThemeSearchPaths(searchPaths);
        themeDebugLog() << "<> Icon themes initialized.";
    }

    // Initialize widget themes
    {
        themeDebugLog() << "<> Initializing Widget Themes";
        themeDebugLog() << "Loading Built-in Theme:" << AddTheme(std::make_unique<SystemTheme>());
        auto darkThemeId = AddTheme(std::make_unique<DarkTheme>());
        themeDebugLog() << "Loading Built-in Theme:" << darkThemeId;
        themeDebugLog() << "Loading Built-in Theme:" << AddTheme(std::make_unique<BrightTheme>());

        // TODO: need some way to differentiate same name themes in different subdirectories (maybe smaller grey text next to theme name in
        // dropdown?)
        QString themeFolder = QDir("./themes/").absoluteFilePath("");
        themeDebugLog() << "Theme Folder Path: " << themeFolder;

        QDirIterator directoryIterator(themeFolder, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (directoryIterator.hasNext()) {
            QDir dir(directoryIterator.next());
            QFileInfo themeJson(dir.absoluteFilePath("theme.json"));
            if (themeJson.exists()) {
                // Load "theme.json" based themes
                themeDebugLog() << "Loading JSON Theme from:" << themeJson.absoluteFilePath();
                AddTheme(std::make_unique<CustomTheme>(GetTheme(darkThemeId), themeJson, true));
            } else {
                // Load pure QSS Themes
                QDirIterator stylesheetFileIterator(dir.absoluteFilePath(""), { "*.qss", "*.css" }, QDir::Files);
                while (stylesheetFileIterator.hasNext()) {
                    QFile customThemeFile(stylesheetFileIterator.next());
                    QFileInfo customThemeFileInfo(customThemeFile);
                    themeDebugLog() << "Loading QSS Theme from:" << customThemeFileInfo.absoluteFilePath();
                    AddTheme(std::make_unique<CustomTheme>(GetTheme(darkThemeId), customThemeFileInfo, false));
                }
            }
        }

        themeDebugLog() << "<> Widget themes initialized.";
    }
}

QList<ITheme*> ThemeManager::getValidApplicationThemes()
{
    QList<ITheme*> ret;
    ret.reserve(m_themes.size());
    for (auto&& [id, theme] : m_themes) {
        ret.append(theme.get());
    }
    return ret;
}

void ThemeManager::setIconTheme(const QString& name)
{
    QIcon::setThemeName(name);
}

void ThemeManager::applyCurrentlySelectedTheme()
{
    setIconTheme(APPLICATION->settings()->get("IconTheme").toString());
    themeDebugLog() << "<> Icon theme set.";
    setApplicationTheme(APPLICATION->settings()->get("ApplicationTheme").toString(), true);
    themeDebugLog() << "<> Application theme set.";
}

void ThemeManager::setApplicationTheme(const QString& name, bool initial)
{
    auto systemPalette = qApp->palette();
    auto themeIter = m_themes.find(name);
    if (themeIter != m_themes.end()) {
        auto& theme = themeIter->second;
        themeDebugLog() << "applying theme" << theme->name();
        theme->apply(initial);
#ifdef Q_OS_WIN
        if (m_mainWindow && IsWindows10OrGreater()) {
            if (QString::compare(theme->id(), "dark") == 0) {
                WinDarkmode::setDarkWinTitlebar(m_mainWindow->winId(), true);
            } else {
                WinDarkmode::setDarkWinTitlebar(m_mainWindow->winId(), false);
            }
        }
#endif
    } else {
        themeWarningLog() << "Tried to set invalid theme:" << name;
    }
}
