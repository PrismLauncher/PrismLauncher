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
#include <pathmatcher/IPathMatcher.h>
#include <QFileSystemWatcher>
#include "ui/pages/BasePage.h"

namespace Ui {
class LogsPage;
}

class RecursiveFileSystemWatcher;
class LogFormatProxyModel;

class LogsPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit LogsPage(InstancePtr instance = nullptr, QWidget* parent = 0);
    ~LogsPage();

    QString id() const override { return "logs"; }
    QString displayName() const override { return tr("Logs"); }
    QIcon icon() const override { return APPLICATION->getThemedIcon("log"); }
    QString helpPage() const override { return "Logs"; }
    void retranslate() override;

    void openedImpl() override;
    void closedImpl() override;

    void selectCurrentLog();
   private:
    void useModel(shared_qobject_ptr<LogModel> model);
    void useModel(LogModel* model);

    QString currentFile() const;

   private slots:
    void populateSelectLogBox();
    void loadLog();
    void loadCurrentLog();
    void loadLogFile(const QString &path);

    void copyClicked();
    void uploadClicked();
    void deleteClicked();
    void clearClicked();
    void cleanUpClicked();

    void keepUpdatingToggled(bool checked);
    void wrapCheckboxToggled(bool checked);
    void colorLinesToggled(bool checked);

    void findClicked();
    void findActivated();
    void findNextActivated();
    void findPreviousActivated();

   private:
    QStringList getPaths() const;

   private:
    Ui::LogsPage* m_ui;
    InstancePtr m_instance;
    /** Path to display log paths relative to. */
    QString m_basePath;
    QStringList m_logSearchPaths;
    QFileSystemWatcher m_watcher;

    LogFormatProxyModel* m_proxy;
    shared_qobject_ptr<LogModel> m_model;
};
