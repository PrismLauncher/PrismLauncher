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
 */
#pragma once

#include <QDir>
#include <QLoggingCategory>
#include <QString>
#include <memory>

#include "IconTheme.h"
#include "ui/themes/CatPack.h"
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
    ThemeManager();

    QList<IconTheme*> getValidIconThemes();
    QList<ITheme*> getValidApplicationThemes();
    bool isValidIconTheme(const QString& id);
    bool isValidApplicationTheme(const QString& id);
    QDir getIconThemesFolder();
    QDir getApplicationThemesFolder();
    QDir getCatPacksFolder();
    void applyCurrentlySelectedTheme(bool initial = false);
    void setIconTheme(const QString& name);
    void setApplicationTheme(const QString& name, bool initial = false);

    /// @brief Returns the background based on selected and with events (Birthday, XMas, etc.)
    /// @param catName Optional, if you need a specific background.
    /// @return
    QString getCatPack(QString catName = "");
    QList<CatPack*> getValidCatPacks();

    const LogColors& getLogColors() { return m_logColors; }

    void refresh();

   private:
    std::map<QString, std::unique_ptr<ITheme>> m_themes;
    std::map<QString, IconTheme> m_icons;
    QDir m_iconThemeFolder{ "iconthemes" };
    QDir m_applicationThemeFolder{ "themes" };
    QDir m_catPacksFolder{ "catpacks" };
    std::map<QString, std::unique_ptr<CatPack>> m_catPacks;
    QString m_defaultStyle;
    QPalette m_defaultPalette;
    LogColors m_logColors;

    void initializeThemes();
    void initializeCatPacks();
    QString addTheme(std::unique_ptr<ITheme> theme);
    ITheme* getTheme(QString themeId);
    QString addIconTheme(IconTheme theme);
    QString addCatPack(std::unique_ptr<CatPack> catPack);
    void initializeIcons();
    void initializeWidgets();

    const QStringList builtinIcons{ "pe_colored", "pe_light", "pe_dark", "pe_blue",    "breeze_light", "breeze_dark",
                                    "OSX",        "iOS",      "flat",    "flat_white", "multimc" };
};
