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

ThemeManager::ThemeManager()
{
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

QString ThemeManager::addIconTheme(IconTheme theme)
{
    QString id = theme.id();
    m_icons.emplace(id, std::move(theme));
    return id;
}

void ThemeManager::initializeThemes()
{
    // Icon themes
    initializeIcons();

    // Initialize widget themes
    initializeWidgets();
}

void ThemeManager::initializeIcons()
{
    // TODO: icon themes and instance icons do not mesh well together. Rearrange and fix discrepancies!
    // set icon theme search path!

    if (!m_iconThemeFolder.mkpath("."))
        themeWarningLog() << "Couldn't create icon theme folder";
    themeDebugLog() << "Icon Theme Folder Path: " << m_iconThemeFolder.absolutePath();

    auto searchPaths = QIcon::themeSearchPaths();
    searchPaths.append(m_iconThemeFolder.path());
    QIcon::setThemeSearchPaths(searchPaths);

    themeDebugLog() << "<> Initializing Icon Themes";

    for (const QString& id : builtinIcons) {
        IconTheme theme(id, QString(":/icons/%1").arg(id));
        if (!theme.load()) {
            themeWarningLog() << "Couldn't load built-in icon theme" << id;
            continue;
        }

        addIconTheme(std::move(theme));
        themeDebugLog() << "Loaded Built-In Icon Theme" << id;
    }

    QDirIterator directoryIterator(m_iconThemeFolder.path(), QDir::Dirs | QDir::NoDotAndDotDot);
    while (directoryIterator.hasNext()) {
        QDir dir(directoryIterator.next());
        IconTheme theme(dir.dirName(), dir.path());
        if (!theme.load())
            continue;

        addIconTheme(std::move(theme));
        themeDebugLog() << "Loaded Custom Icon Theme from" << dir.path();
    }

    themeDebugLog() << "<> Icon themes initialized.";
}

void ThemeManager::initializeWidgets()
{
    themeDebugLog() << "<> Initializing Widget Themes";
    themeDebugLog() << "Loading Built-in Theme:" << addTheme(std::make_unique<SystemTheme>());
    auto darkThemeId = addTheme(std::make_unique<DarkTheme>());
    themeDebugLog() << "Loading Built-in Theme:" << darkThemeId;
    themeDebugLog() << "Loading Built-in Theme:" << addTheme(std::make_unique<BrightTheme>());

    // TODO: need some way to differentiate same name themes in different subdirectories (maybe smaller grey text next to theme name in
    // dropdown?)

    if (!m_applicationThemeFolder.mkpath("."))
        themeWarningLog() << "Couldn't create theme folder";
    themeDebugLog() << "Theme Folder Path: " << m_applicationThemeFolder.absolutePath();

    QDirIterator directoryIterator(m_applicationThemeFolder.path(), QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (directoryIterator.hasNext()) {
        QDir dir(directoryIterator.next());
        QFileInfo themeJson(dir.absoluteFilePath("theme.json"));
        if (themeJson.exists()) {
            // Load "theme.json" based themes
            themeDebugLog() << "Loading JSON Theme from:" << themeJson.absoluteFilePath();
            addTheme(std::make_unique<CustomTheme>(getTheme(darkThemeId), themeJson, true));
        } else {
            // Load pure QSS Themes
            QDirIterator stylesheetFileIterator(dir.absoluteFilePath(""), { "*.qss", "*.css" }, QDir::Files);
            while (stylesheetFileIterator.hasNext()) {
                QFile customThemeFile(stylesheetFileIterator.next());
                QFileInfo customThemeFileInfo(customThemeFile);
                themeDebugLog() << "Loading QSS Theme from:" << customThemeFileInfo.absoluteFilePath();
                addTheme(std::make_unique<CustomTheme>(getTheme(darkThemeId), customThemeFileInfo, false));
            }
        }
    }

    themeDebugLog() << "<> Widget themes initialized.";
}

QList<IconTheme*> ThemeManager::getValidIconThemes()
{
    QList<IconTheme*> ret;
    ret.reserve(m_icons.size());
    for (auto&& [id, theme] : m_icons) {
        ret.append(&theme);
    }
    return ret;
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

bool ThemeManager::isValidIconTheme(const QString& id)
{
    return !id.isEmpty() && m_icons.find(id) != m_icons.end();
}

bool ThemeManager::isValidApplicationTheme(const QString& id)
{
    return !id.isEmpty() && m_themes.find(id) != m_themes.end();
}

QDir ThemeManager::getIconThemesFolder()
{
    return m_iconThemeFolder;
}

QDir ThemeManager::getApplicationThemesFolder()
{
    return m_applicationThemeFolder;
}

void ThemeManager::setIconTheme(const QString& name)
{
    if (m_icons.find(name) == m_icons.end()) {
        themeWarningLog() << "Tried to set invalid icon theme:" << name;
        return;
    }

    QIcon::setThemeName(name);
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

void ThemeManager::applyCurrentlySelectedTheme(bool initial)
{
    auto settings = APPLICATION->settings();
    setIconTheme(settings->get("IconTheme").toString());
    themeDebugLog() << "<> Icon theme set.";
    setApplicationTheme(settings->get("ApplicationTheme").toString(), initial);
    themeDebugLog() << "<> Application theme set.";
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
