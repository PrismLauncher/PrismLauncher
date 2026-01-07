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

#include <QWidget>

#include <Application.h>
#include <QFileSystemWatcher>
#include "LogPage.h"
#include "ui/pages/BasePage.h"

namespace Ui {
class OtherLogsPage;
}

class RecursiveFileSystemWatcher;

class OtherLogsPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit OtherLogsPage(QString id, QString displayName, QString helpPage, BaseInstance* instance = nullptr, QWidget* parent = 0);
    ~OtherLogsPage();

    QString id() const override { return m_id; }
    QString displayName() const override { return m_displayName; }
    QIcon icon() const override { return QIcon::fromTheme("log"); }
    QString helpPage() const override { return m_helpPage; }
    void retranslate() override;

    void openedImpl() override;
    void closedImpl() override;

   private slots:
    void populateSelectLogBox();
    void on_selectLogBox_currentIndexChanged(int index);
    void on_btnReload_clicked();
    void on_btnPaste_clicked();
    void on_btnCopy_clicked();
    void on_btnDelete_clicked();
    void on_btnClean_clicked();
    void on_btnBottom_clicked();

    void on_trackLogCheckbox_clicked(bool checked);
    void on_wrapCheckbox_clicked(bool checked);
    void on_colorCheckbox_clicked(bool checked);

    void on_findButton_clicked();
    void findActivated();
    void findNextActivated();
    void findPreviousActivated();

   private:
    void reload();
    void modelStateToUI();
    void UIToModelState();
    void setControlsEnabled(bool enabled);

    QStringList getPaths();

   private:
    QString m_id;
    QString m_displayName;
    QString m_helpPage;

    Ui::OtherLogsPage* ui;
    BaseInstance* m_instance;
    /** Path to display log paths relative to. */
    QString m_basePath;
    QStringList m_logSearchPaths;
    QString m_currentFile;
    QFileSystemWatcher m_watcher;

    LogFormatProxyModel* m_proxy;
    LogModel* m_model;
};
