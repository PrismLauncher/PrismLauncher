// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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

#pragma once

#include <QDialog>
#include <memory>

#include <Application.h>
#include <translations/TranslationsModel.h>
#include "java/JavaChecker.h"
#include "ui/pages/BasePage.h"

class QTextCharFormat;
class SettingsObject;

namespace Ui {
class LauncherPage;
}

class LauncherPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit LauncherPage(QWidget* parent = 0);
    ~LauncherPage();

    QString displayName() const override { return tr("Launcher"); }
    QIcon icon() const override { return APPLICATION->getThemedIcon("launcher"); }
    QString id() const override { return "launcher-settings"; }
    QString helpPage() const override { return "Launcher-settings"; }
    bool apply() override;
    void retranslate() override;

   private:
    void applySettings();
    void loadSettings();

   private slots:
    void on_instDirBrowseBtn_clicked();
    void on_modsDirBrowseBtn_clicked();
    void on_iconsDirBrowseBtn_clicked();
    void on_downloadsDirBrowseBtn_clicked();
    void on_javaDirBrowseBtn_clicked();
    void on_skinsDirBrowseBtn_clicked();
    void on_metadataDisableBtn_clicked();

    /*!
     * Updates the font preview
     */
    void refreshFontPreview();

   private:
    Ui::LauncherPage* ui;

    /*!
     * Stores the currently selected update channel.
     */
    QString m_currentUpdateChannel;

    // default format for the font preview...
    QTextCharFormat* defaultFormat;

    std::shared_ptr<TranslationsModel> m_languageModel;
};
