// SPDX-FileCopyrightText: 2023 flow <flowlnlnln@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <FileSystem.h>

#include <ui/themes/FusionTheme.h>
#include <ui/themes/ThemeManager.h>

class TemporaryGlobalQMLConfigFile {
   public:
    TemporaryGlobalQMLConfigFile(QString conf_file_path, bool append_conf) : m_conf_file_path(std::move(conf_file_path))
    {
        if (append_conf)
            this->m_conf_file_path = FS::PathCombine(this->m_conf_file_path, "qtquickcontrols2.conf");

        if (QFile::exists(".qml_theme"))
            QFile::rename(".qml_theme", ".qml_theme.bkp");

        ThemeManager::writeGlobalQMLTheme(this->m_conf_file_path);
    }

    ~TemporaryGlobalQMLConfigFile()
    {
        QFile::remove(".qml_theme");
        QFile::rename(".qml_theme.bkp", ".qml_theme");
    }

    QString m_conf_file_path;
};

void setupSystemTheme()
{
    qunsetenv("QT_QUICK_CONTROLS_CONF");
    ThemeManager::bootstrapThemeEnvironment();
}

auto setupDefaultFusionTheme()
{
    TemporaryGlobalQMLConfigFile t(FusionTheme::USE_FUSION_QML_GLOBAL_THEME, false);
    ThemeManager::bootstrapThemeEnvironment();
    return t;
}

auto setupCustomTheme(QString const& theme_path)
{
    TemporaryGlobalQMLConfigFile t(theme_path, true);
    ThemeManager::bootstrapThemeEnvironment();
    return t;
}
