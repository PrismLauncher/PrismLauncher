// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2024 Tayou <git@tayou.org>
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
#include "CustomTheme.h"
#include <FileSystem.h>
#include <Json.h>
#include "ThemeManager.h"

const char* themeFile = "theme.json";

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

        m_palette = baseTheme->colorScheme();

        try {
            bool hasCustomLogColors = false;
            auto rsp = read(themeFilePath, hasCustomLogColors);
            if (!rsp) {
                themeWarningLog() << "Couldn't read theme json:" << rsp.error();
                m_logColors = defaultLogColors(m_palette);
                m_styleSheet = baseTheme->appStyleSheet();
            } else {
                // If theme data was found, fade "Disabled" color of each role according to FadeAmount
                m_palette = fadeInactive(m_palette, m_fadeAmount, m_fadeColor);
                if (!hasCustomLogColors) {
                    m_logColors = defaultLogColors(m_palette);
                }
            }
        } catch (const Exception& e) {
            themeWarningLog() << "Couldn't load theme json:" << e.cause();
            m_logColors = defaultLogColors(m_palette);
            m_styleSheet = baseTheme->appStyleSheet();
            return;
        }

        auto qssFilePath = FS::PathCombine(path, m_qssFilePath);
        QFileInfo info(qssFilePath);
        if (info.isFile()) {
            // TODO: validate qss?
            auto rsp = FS::read(qssFilePath);
            if (!rsp) {
                themeWarningLog() << "Couldn't load qss:" << rsp.error() << "from" << qssFilePath;
                m_styleSheet = baseTheme->appStyleSheet();
            } else {
                m_styleSheet = QString::fromUtf8(rsp.value());
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
            themeWarningLog().nospace() << m_name << ": Theme file path doesn't exist!";
            m_palette = baseTheme->colorScheme();
            m_styleSheet = baseTheme->appStyleSheet();
            return;
        }

        m_palette = baseTheme->colorScheme();
        // TODO: validate qss?
        auto rsp = FS::read(path);
        if (!rsp) {
            themeWarningLog() << "Couldn't load qss:" << rsp.error() << "from" << path;
            m_styleSheet = baseTheme->appStyleSheet();
        } else {
            m_styleSheet = QString::fromUtf8(rsp.value());
        }
    }
}

QStringList CustomTheme::searchPaths()
{
    QString pathResources = FS::PathCombine("themes", m_id, "resources");
    if (QFileInfo::exists(pathResources)) {
        return { pathResources };
    }

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

Result<> CustomTheme::read(const QString& path, bool& hasCustomLogColors)
{
    QFileInfo pathInfo(path);
    if (!pathInfo.exists() || !pathInfo.isFile()) {
        themeDebugLog() << "No theme json present.";
        return std::unexpected(QStringLiteral("Theme json file does not exist"));
    }

    auto doc = Json::requireDocument(path, "Theme JSON file");
    TRY(doc)
    const QJsonObject root = doc.value().object();
    m_name = Json::requireString(root, "name", "Theme name");
    m_widgets = Json::requireString(root, "widgets", "Qt widget theme");
    m_qssFilePath = root["qssFilePath"].toString("themeStyle.css");

    auto readColor = [](const QJsonObject& colors, const QString& colorName) -> QColor {
        auto colorValue = colors[colorName].toString();
        if (!colorValue.isEmpty()) {
            QColor color(colorValue);
            if (!color.isValid()) {
                themeWarningLog() << "Color value" << colorValue << "for" << colorName << "was not recognized.";
                return {};
            }
            return color;
        }
        return {};
    };

    if (root.contains("colors")) {
        auto colorsRoot = Json::requireObject(root, "colors");
        auto readAndSetPaletteColor = [this, readColor, colorsRoot](QPalette::ColorRole role, const QString& colorName) {
            auto color = readColor(colorsRoot, colorName);
            if (color.isValid()) {
                m_palette.setColor(role, color);
            } else {
                themeDebugLog() << "Color value for" << colorName << "was not present.";
            }
        };

        // palette
        readAndSetPaletteColor(QPalette::Window, "Window");
        readAndSetPaletteColor(QPalette::WindowText, "WindowText");
        readAndSetPaletteColor(QPalette::Base, "Base");
        readAndSetPaletteColor(QPalette::AlternateBase, "AlternateBase");
        readAndSetPaletteColor(QPalette::ToolTipBase, "ToolTipBase");
        readAndSetPaletteColor(QPalette::ToolTipText, "ToolTipText");
        readAndSetPaletteColor(QPalette::Text, "Text");
        readAndSetPaletteColor(QPalette::Button, "Button");
        readAndSetPaletteColor(QPalette::ButtonText, "ButtonText");
        readAndSetPaletteColor(QPalette::BrightText, "BrightText");
        readAndSetPaletteColor(QPalette::Link, "Link");
        readAndSetPaletteColor(QPalette::Highlight, "Highlight");
        readAndSetPaletteColor(QPalette::HighlightedText, "HighlightedText");

        // fade
        m_fadeColor = readColor(colorsRoot, "fadeColor");
        m_fadeAmount = colorsRoot["fadeAmount"].toDouble(0.5);
    }

    if (root.contains("logColors")) {
        hasCustomLogColors = true;

        auto logColorsRoot = Json::requireObject(root, "logColors");
        auto readAndSetLogColor = [this, readColor, logColorsRoot](MessageLevel level, bool fg, const QString& colorName) {
            auto color = readColor(logColorsRoot, colorName);
            if (color.isValid()) {
                if (fg) {
                    m_logColors.foreground[level] = color;
                } else {
                    m_logColors.background[level] = color;
                }
            } else {
                themeDebugLog() << "Color value for" << colorName << "was not present.";
            }
        };

        readAndSetLogColor(MessageLevel::Message, false, "MessageHighlight");
        readAndSetLogColor(MessageLevel::Launcher, false, "LauncherHighlight");
        readAndSetLogColor(MessageLevel::Debug, false, "DebugHighlight");
        readAndSetLogColor(MessageLevel::Warning, false, "WarningHighlight");
        readAndSetLogColor(MessageLevel::Error, false, "ErrorHighlight");
        readAndSetLogColor(MessageLevel::Fatal, false, "FatalHighlight");

        readAndSetLogColor(MessageLevel::Message, true, "Message");
        readAndSetLogColor(MessageLevel::Launcher, true, "Launcher");
        readAndSetLogColor(MessageLevel::Debug, true, "Debug");
        readAndSetLogColor(MessageLevel::Warning, true, "Warning");
        readAndSetLogColor(MessageLevel::Error, true, "Error");
        readAndSetLogColor(MessageLevel::Fatal, true, "Fatal");
    }
    return {};
}
