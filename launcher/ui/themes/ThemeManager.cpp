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
#include <QImageReader>
#include "Exception.h"
#include "ui/themes/BrightTheme.h"
#include "ui/themes/CatPack.h"
#include "ui/themes/CustomTheme.h"
#include "ui/themes/DarkTheme.h"
#include "ui/themes/SystemTheme.h"

#include "Application.h"

ThemeManager::ThemeManager(MainWindow* mainWindow)
{
    m_mainWindow = mainWindow;
    initializeThemes();
    initializeCatPacks();
}

/// @brief Adds the Theme to the list of themes
/// @param theme The Theme to add
/// @return Theme ID
QString ThemeManager::addTheme(std::unique_ptr<ITheme> theme)
{
    QString id = theme->id();
    if (m_themes.find(id) == m_themes.end())
        m_themes.emplace(id, std::move(theme));
    else
        themeWarningLog() << "Theme(" << id << ") not added to prevent id duplication";
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

QList<CatPack*> ThemeManager::getValidCatPacks()
{
    QList<CatPack*> ret;
    ret.reserve(m_catPacks.size());
    for (auto&& [id, theme] : m_catPacks) {
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

QString ThemeManager::getCatPack(QString catName)
{
    auto catIter = m_catPacks.find(!catName.isEmpty() ? catName : APPLICATION->settings()->get("BackgroundCat").toString());
    if (catIter != m_catPacks.end()) {
        auto& catPack = catIter->second;
        themeDebugLog() << "applying catpack" << catPack->id();
        return catPack->path();
    } else {
        themeWarningLog() << "Tried to get invalid catPack:" << catName;
    }

    return m_catPacks.begin()->second->path();
}

QString ThemeManager::addCatPack(std::unique_ptr<CatPack> catPack)
{
    QString id = catPack->id();
    if (m_catPacks.find(id) == m_catPacks.end())
        m_catPacks.emplace(id, std::move(catPack));
    else
        themeWarningLog() << "CatPack(" << id << ") not added to prevent id duplication";
    return id;
}

void ThemeManager::initializeCatPacks()
{
    QList<std::pair<QString, QString>> defaultCats{ { "kitteh", QObject::tr("Background Cat (from MultiMC)") },
                                                    { "rory", QObject::tr("Rory ID 11 (drawn by Ashtaka)") },
                                                    { "rory-flat", QObject::tr("Rory ID 11 (flat edition, drawn by Ashtaka)") },
                                                    { "teawie", QObject::tr("Teawie (drawn by SympathyTea)") } };
    for (auto [id, name] : defaultCats) {
        addCatPack(std::unique_ptr<CatPack>(new BasicCatPack(id, name)));
    }
    QDir catpacksDir("catpacks");
    QString catpacksFolder = catpacksDir.absoluteFilePath("");
    themeDebugLog() << "CatPacks Folder Path:" << catpacksFolder;

    QStringList supportedImageFormats;
    for (auto format : QImageReader::supportedImageFormats()) {
        supportedImageFormats.append("*." + format);
    }
    auto loadFiles = [this, supportedImageFormats](QDir dir) {
        // Load image files directly
        QDirIterator ImageFileIterator(dir.absoluteFilePath(""), supportedImageFormats, QDir::Files);
        while (ImageFileIterator.hasNext()) {
            QFile customCatFile(ImageFileIterator.next());
            QFileInfo customCatFileInfo(customCatFile);
            themeDebugLog() << "Loading CatPack from:" << customCatFileInfo.absoluteFilePath();
            addCatPack(std::unique_ptr<CatPack>(new FileCatPack(customCatFileInfo)));
        }
    };

    loadFiles(catpacksDir);

    QDirIterator directoryIterator(catpacksFolder, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (directoryIterator.hasNext()) {
        QDir dir(directoryIterator.next());
        QFileInfo manifest(dir.absoluteFilePath("catpack.json"));
        if (manifest.isFile()) {
            try {
                // Load background manifest
                themeDebugLog() << "Loading background manifest from:" << manifest.absoluteFilePath();
                addCatPack(std::unique_ptr<CatPack>(new JsonCatPack(manifest)));
            } catch (const Exception& e) {
                themeWarningLog() << "Couldn't load catpack json:" << e.cause();
            }
        } else {
            loadFiles(dir);
        }
    }
}
