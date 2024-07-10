// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2024 Tayou <git@tayou.org>
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
#include "CustomTheme.h"
#include <FileSystem.h>
#include <Json.h>
#include "ThemeManager.h"

const char* themeFile = "theme.json";

static bool readThemeJson(const QString& path,
                          QPalette& palette,
                          double& fadeAmount,
                          QColor& fadeColor,
                          QString& name,
                          QString& widgets,
                          QString& qssFilePath,
                          bool& dataIncomplete)
{
    QFileInfo pathInfo(path);
    if (pathInfo.exists() && pathInfo.isFile()) {
        try {
            auto doc = Json::requireDocument(path, "Theme JSON file");
            const QJsonObject root = doc.object();
            dataIncomplete = !root.contains("qssFilePath");
            name = Json::requireString(root, "name", "Theme name");
            widgets = Json::requireString(root, "widgets", "Qt widget theme");
            qssFilePath = Json::ensureString(root, "qssFilePath", "themeStyle.css");
            auto colorsRoot = Json::requireObject(root, "colors", "colors object");
            auto readColor = [&](QString colorName) -> QColor {
                auto colorValue = Json::ensureString(colorsRoot, colorName, QString());
                if (!colorValue.isEmpty()) {
                    QColor color(colorValue);
                    if (!color.isValid()) {
                        themeWarningLog() << "Color value" << colorValue << "for" << colorName << "was not recognized.";
                        return QColor();
                    }
                    return color;
                }
                return QColor();
            };
            auto readAndSetColor = [&](QPalette::ColorRole role, QString colorName) {
                auto color = readColor(colorName);
                if (color.isValid()) {
                    palette.setColor(role, color);
                } else {
                    themeDebugLog() << "Color value for" << colorName << "was not present.";
                }
            };

            // palette
            readAndSetColor(QPalette::Window, "Window");
            readAndSetColor(QPalette::WindowText, "WindowText");
            readAndSetColor(QPalette::Base, "Base");
            readAndSetColor(QPalette::AlternateBase, "AlternateBase");
            readAndSetColor(QPalette::ToolTipBase, "ToolTipBase");
            readAndSetColor(QPalette::ToolTipText, "ToolTipText");
            readAndSetColor(QPalette::Text, "Text");
            readAndSetColor(QPalette::Button, "Button");
            readAndSetColor(QPalette::ButtonText, "ButtonText");
            readAndSetColor(QPalette::BrightText, "BrightText");
            readAndSetColor(QPalette::Link, "Link");
            readAndSetColor(QPalette::Highlight, "Highlight");
            readAndSetColor(QPalette::HighlightedText, "HighlightedText");

            // fade
            fadeColor = readColor("fadeColor");
            fadeAmount = Json::ensureDouble(colorsRoot, "fadeAmount", 0.5, "fade amount");

        } catch (const Exception& e) {
            themeWarningLog() << "Couldn't load theme json: " << e.cause();
            return false;
        }
    } else {
        themeDebugLog() << "No theme json present.";
        return false;
    }
    return true;
}

static bool writeThemeJson(const QString& path,
                           const QPalette& palette,
                           double fadeAmount,
                           QColor fadeColor,
                           QString name,
                           QString widgets,
                           QString qssFilePath)
{
    QJsonObject rootObj;
    rootObj.insert("name", name);
    rootObj.insert("widgets", widgets);
    rootObj.insert("qssFilePath", qssFilePath);

    QJsonObject colorsObj;
    auto insertColor = [&](QPalette::ColorRole role, QString colorName) { colorsObj.insert(colorName, palette.color(role).name()); };

    // palette
    insertColor(QPalette::Window, "Window");
    insertColor(QPalette::WindowText, "WindowText");
    insertColor(QPalette::Base, "Base");
    insertColor(QPalette::AlternateBase, "AlternateBase");
    insertColor(QPalette::ToolTipBase, "ToolTipBase");
    insertColor(QPalette::ToolTipText, "ToolTipText");
    insertColor(QPalette::Text, "Text");
    insertColor(QPalette::Button, "Button");
    insertColor(QPalette::ButtonText, "ButtonText");
    insertColor(QPalette::BrightText, "BrightText");
    insertColor(QPalette::Link, "Link");
    insertColor(QPalette::Highlight, "Highlight");
    insertColor(QPalette::HighlightedText, "HighlightedText");

    // fade
    colorsObj.insert("fadeColor", fadeColor.name());
    colorsObj.insert("fadeAmount", fadeAmount);

    rootObj.insert("colors", colorsObj);
    try {
        Json::write(rootObj, path);
        return true;
    } catch ([[maybe_unused]] const Exception& e) {
        themeWarningLog() << "Failed to write theme json to" << path;
        return false;
    }
}

/// @param baseTheme Base Theme
/// @param fileInfo FileInfo object for file to load
/// @param isManifest whether to load a theme manifest or a qss file
CustomTheme::CustomTheme(ITheme* baseTheme, QFileInfo& fileInfo, bool isManifest)
{
    if (isManifest) {
        m_id = fileInfo.dir().dirName();

        QString path = FS::PathCombine("themes", m_id);
        QString pathResources = FS::PathCombine("themes", m_id, "resources");

        if (!FS::ensureFolderPathExists(path)) {
            themeWarningLog() << "Theme directory for" << m_id << "could not be created. This theme might be invalid";
            return;
        }

        if (!FS::ensureFolderPathExists(pathResources)) {
            themeWarningLog() << "Resources directory for" << m_id << "could not be created";
        }

        auto themeFilePath = FS::PathCombine(path, themeFile);

        bool jsonDataIncomplete = false;

        m_palette = baseTheme->colorScheme();
        if (readThemeJson(themeFilePath, m_palette, m_fadeAmount, m_fadeColor, m_name, m_widgets, m_qssFilePath, jsonDataIncomplete)) {
            // If theme data was found, fade "Disabled" color of each role according to FadeAmount
            m_palette = fadeInactive(m_palette, m_fadeAmount, m_fadeColor);
        } else {
            themeDebugLog() << "Did not read theme json file correctly, not changing theme, keeping previous.";
            return;
        }

        // FIXME: This is kinda jank, it only actually checks if the qss file path is not present. It should actually check for any relevant
        // missing data (e.g. name, colors)
        if (jsonDataIncomplete) {
            writeThemeJson(fileInfo.absoluteFilePath(), m_palette, m_fadeAmount, m_fadeColor, m_name, m_widgets, m_qssFilePath);
        }

        auto qssFilePath = FS::PathCombine(path, m_qssFilePath);
        QFileInfo info(qssFilePath);
        if (info.isFile()) {
            try {
                // TODO: validate qss?
                m_styleSheet = QString::fromUtf8(FS::read(qssFilePath));
            } catch (const Exception& e) {
                themeWarningLog() << "Couldn't load qss:" << e.cause() << "from" << qssFilePath;
                return;
            }
        } else {
            themeDebugLog() << "No theme qss present.";
        }
    } else {
        m_id = fileInfo.fileName();
        m_name = fileInfo.baseName();
        QString path = fileInfo.filePath();
        // themeDebugLog << "Theme ID: " << m_id;
        // themeDebugLog << "Theme Name: " << m_name;
        // themeDebugLog << "Theme Path: " << path;

        if (!FS::ensureFilePathExists(path)) {
            themeWarningLog() << m_name << " Theme file path doesn't exist!";
            m_palette = baseTheme->colorScheme();
            m_styleSheet = baseTheme->appStyleSheet();
            return;
        }

        m_palette = baseTheme->colorScheme();
        try {
            // TODO: validate qss?
            m_styleSheet = QString::fromUtf8(FS::read(path));
        } catch (const Exception& e) {
            themeWarningLog() << "Couldn't load qss:" << e.cause() << "from" << path;
            m_styleSheet = baseTheme->appStyleSheet();
        }
    }
}

QStringList CustomTheme::searchPaths()
{
    QString pathResources = FS::PathCombine("themes", m_id, "resources");
    if (QFileInfo::exists(pathResources))
        return { pathResources };

    return {};
}

QString CustomTheme::id()
{
    return m_id;
}

QString CustomTheme::name()
{
    return m_name;
}

bool CustomTheme::hasColorScheme()
{
    return true;
}

QPalette CustomTheme::colorScheme()
{
    return m_palette;
}

bool CustomTheme::hasStyleSheet()
{
    return true;
}

QString CustomTheme::appStyleSheet()
{
    return m_styleSheet;
}

double CustomTheme::fadeAmount()
{
    return m_fadeAmount;
}

QColor CustomTheme::fadeColor()
{
    return m_fadeColor;
}

QString CustomTheme::qtTheme()
{
    return m_widgets;
}
QString CustomTheme::tooltip()
{
    return m_tooltip;
}
