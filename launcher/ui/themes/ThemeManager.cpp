// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2024 Tayou <git@tayou.org>
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
#include <QImageReader>
#include <QStyle>
#include <QStyleFactory>
#include "Exception.h"
#include "ui/themes/BrightTheme.h"
#include "ui/themes/CatPack.h"
#include "ui/themes/CustomTheme.h"
#include "ui/themes/DarkTheme.h"
#include "ui/themes/SystemTheme.h"

#include "Application.h"

ThemeManager::ThemeManager()
{
    QIcon::setFallbackThemeName(QIcon::themeName());
    QIcon::setThemeSearchPaths(QIcon::themeSearchPaths() << m_iconThemeFolder.path());

    themeDebugLog() << "Determining System Widget Theme...";
    const auto& style = QApplication::style();
    m_defaultStyle = style->objectName();
    themeDebugLog() << "System theme seems to be:" << m_defaultStyle;

    m_defaultPalette = QApplication::palette();

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

QString ThemeManager::addIconTheme(IconTheme theme)
{
    QString id = theme.id();
    if (m_icons.find(id) == m_icons.end())
        m_icons.emplace(id, std::move(theme));
    else
        themeWarningLog() << "IconTheme(" << id << ") not added to prevent id duplication";
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

    if (!m_iconThemeFolder.mkpath("."))
        themeWarningLog() << "Couldn't create icon theme folder";
    themeDebugLog() << "Icon Theme Folder Path: " << m_iconThemeFolder.absolutePath();

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
    themeDebugLog() << "Loading Built-in Theme:" << addTheme(std::make_unique<SystemTheme>(m_defaultStyle, m_defaultPalette, true));
    auto darkThemeId = addTheme(std::make_unique<DarkTheme>());
    themeDebugLog() << "Loading Built-in Theme:" << darkThemeId;
    themeDebugLog() << "Loading Built-in Theme:" << addTheme(std::make_unique<BrightTheme>());

    themeDebugLog() << "<> Initializing System Widget Themes";
    QStringList styles = QStyleFactory::keys();
    for (auto& st : styles) {
#ifdef Q_OS_WINDOWS
        if (QSysInfo::productVersion() != "11" && st == "windows11") {
            continue;
        }
#endif
        themeDebugLog() << "Loading System Theme:" << addTheme(std::make_unique<SystemTheme>(st, m_defaultPalette, false));
    }

    // TODO: need some way to differentiate same name themes in different subdirectories
    //  (maybe smaller grey text next to theme name in dropdown?)

    if (!m_applicationThemeFolder.mkpath("."))
        themeWarningLog() << "Couldn't create theme folder";
    themeDebugLog() << "Theme Folder Path: " << m_applicationThemeFolder.absolutePath();

    QDirIterator directoryIterator(m_applicationThemeFolder.path(), QDir::Dirs | QDir::NoDotAndDotDot);
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

QList<CatPack*> ThemeManager::getValidCatPacks()
{
    QList<CatPack*> ret;
    ret.reserve(m_catPacks.size());
    for (auto&& [id, theme] : m_catPacks) {
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

QDir ThemeManager::getCatPacksFolder()
{
    return m_catPacksFolder;
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

        m_logColors = theme->logColorScheme();
    } else {
        themeWarningLog() << "Tried to set invalid theme:" << name;
    }
}

void ThemeManager::applyCurrentlySelectedTheme(bool initial)
{
    auto settings = APPLICATION->settings();
    setIconTheme(settings->get("IconTheme").toString());
    themeDebugLog() << "<> Icon theme set.";
    auto applicationTheme = settings->get("ApplicationTheme").toString();
    if (applicationTheme == "") {
        applicationTheme = m_defaultStyle;
    }
    setApplicationTheme(applicationTheme, initial);
    themeDebugLog() << "<> Application theme set.";
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
    if (!m_catPacksFolder.mkpath("."))
        themeWarningLog() << "Couldn't create catpacks folder";
    themeDebugLog() << "CatPacks Folder Path:" << m_catPacksFolder.absolutePath();

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

    loadFiles(m_catPacksFolder);

    QDirIterator directoryIterator(m_catPacksFolder.path(), QDir::Dirs | QDir::NoDotAndDotDot);
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

void ThemeManager::refresh()
{
    m_themes.clear();
    m_icons.clear();
    m_catPacks.clear();

    initializeThemes();
    initializeCatPacks();
}
