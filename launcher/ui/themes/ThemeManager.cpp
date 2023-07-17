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

#include <fstream>
#include <iostream>

#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QIcon>
#include <QQuickStyle>

#include "Application.h"

#include "ui/themes/BrightTheme.h"
#include "ui/themes/CustomTheme.h"
#include "ui/themes/DarkTheme.h"
#include "ui/themes/SystemTheme.h"

ThemeManager::ThemeManager(MainWindow* mainWindow)
{
    m_mainWindow = mainWindow;
    initializeThemes();
}

/// @brief Adds the Theme to the list of themes
/// @param theme The Theme to add
/// @return Theme ID
QString ThemeManager::addTheme(std::unique_ptr<ITheme> theme)
{
    QString id = theme->id();
    m_themes.emplace(id, std::move(theme));
    return id;
}

/// @brief Gets the Theme from the List via ID
/// @param themeId Theme ID of theme to fetch
/// @return Theme at themeId
ITheme* ThemeManager::getTheme(QString themeId)
{
    return m_themes[themeId].get();
}

void ThemeManager::initializeThemes()
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
        themeDebugLog() << "Loading Built-in Theme:" << addTheme(std::make_unique<SystemTheme>());
        auto darkThemeId = addTheme(std::make_unique<DarkTheme>());
        themeDebugLog() << "Loading Built-in Theme:" << darkThemeId;
        themeDebugLog() << "Loading Built-in Theme:" << addTheme(std::make_unique<BrightTheme>());

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
                addTheme(std::make_unique<CustomTheme>(getTheme(darkThemeId), themeJson, dir, true));
            } else {
                // Load pure QSS Themes
                QDirIterator stylesheetFileIterator(dir.absoluteFilePath(""), { "*.qss", "*.css" }, QDir::Files);
                while (stylesheetFileIterator.hasNext()) {
                    QFile customThemeFile(stylesheetFileIterator.next());
                    QFileInfo customThemeFileInfo(customThemeFile);
                    themeDebugLog() << "Loading QSS Theme from:" << customThemeFileInfo.absoluteFilePath();
                    addTheme(std::make_unique<CustomTheme>(getTheme(darkThemeId), customThemeFileInfo, dir, false));
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

void ThemeManager::applyCurrentlySelectedTheme(bool initial)
{
    setIconTheme(APPLICATION->settings()->get("IconTheme").toString());
    themeDebugLog() << "<> Icon theme set.";
    setApplicationTheme(APPLICATION->settings()->get("ApplicationTheme").toString(), initial);
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
    } else {
        themeWarningLog() << "Tried to set invalid theme:" << name;
    }
}

bool ThemeManager::needsRestart() const
{
    auto theme_name = APPLICATION->settings()->get("ApplicationTheme").toString();
    auto themeIter = m_themes.find(theme_name);
    if (themeIter != m_themes.end())
        return themeIter->second->changed_qqc_theme;
    return false;
}

QString ThemeManager::getCatImage(QString catName)
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime birthday(QDate(now.date().year(), 11, 30), QTime(0, 0));
    QDateTime xmas(QDate(now.date().year(), 12, 25), QTime(0, 0));
    QDateTime halloween(QDate(now.date().year(), 10, 31), QTime(0, 0));
    QString cat = !catName.isEmpty() ? catName : APPLICATION->settings()->get("BackgroundCat").toString();
    if (std::abs(now.daysTo(xmas)) <= 4) {
        cat += "-xmas";
    } else if (std::abs(now.daysTo(halloween)) <= 4) {
        cat += "-spooky";
    } else if (std::abs(now.daysTo(birthday)) <= 12) {
        cat += "-bday";
    }
    return cat;
}

#define QML_THEME_FILE ".qml_theme"

bool ThemeManager::s_is_qml_system_theme = false;

void ThemeManager::bootstrapThemeEnvironment()
{
    auto initial_conf_path = qgetenv("QT_QUICK_CONTROLS_CONF");
    QString current_conf_path;

    { // Read the theme from the global file
        std::ifstream qml_theme_file(QML_THEME_FILE);
        std::string line;
        if (qml_theme_file.is_open()) {
            std::getline(qml_theme_file, line);
            current_conf_path = QString::fromLocal8Bit(line.data());

            qml_theme_file.close();
        } else {
            std::cout << "No QML theme file could be found!" << std::endl;
        }
    }

    if (current_conf_path.isEmpty()) {
        // System theme: Restore the previous value and leave the style unchanged
        s_is_qml_system_theme = true;
        if (!initial_conf_path.isEmpty())
            qputenv("QT_QUICK_CONTROLS_CONF", initial_conf_path);
        else
            qunsetenv("QT_QUICK_CONTROLS_CONF");
    } else {
        // Not system theme: May be an actual path (custom theme), or one of the built-in ones
        s_is_qml_system_theme = false;
        if (current_conf_path == FusionTheme::USE_FUSION_QML_GLOBAL_THEME) {
            // One of the built-in themes, set the theme to Fusion
            QQuickStyle::setStyle("Fusion");

            return;
        }

        qputenv("QT_QUICK_CONTROLS_CONF", current_conf_path.toLocal8Bit());
    }

    // FIXME: This disallows setting a fallback style on qtquickcontrols2.conf.
    QQuickStyle::setFallbackStyle("Fusion");
}

void ThemeManager::writeGlobalQMLTheme(QString const& conf_file_path)
{
    QFile global_theme_file(QML_THEME_FILE);
    global_theme_file.open(QFile::OpenModeFlag::WriteOnly | QFile::OpenModeFlag::Truncate);

    if (conf_file_path.isEmpty())
        themeDebugLog() << "Clearing QQC config...";
    else
        themeDebugLog() << "Setting QQC config to:" << conf_file_path;

    global_theme_file.write(conf_file_path.toLocal8Bit());

    global_theme_file.close();
}
