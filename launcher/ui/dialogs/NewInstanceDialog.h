// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include "InstanceTask.h"
#include "ui/pages/BasePageProvider.h"

namespace Ui {
class NewInstanceDialog;
}

class PageContainer;
class QDialogButtonBox;
class ImportPage;
class FlamePage;

class NewInstanceDialog : public QDialog, public BasePageProvider {
    Q_OBJECT

   public:
    explicit NewInstanceDialog(const QString& initialGroup,
                               const QString& url = QString(),
                               const QMap<QString, QString>& extra_info = {},
                               QWidget* parent = 0);
    ~NewInstanceDialog();

    void updateDialogState();

    void setSuggestedPack(const QString& name = QString(), InstanceTask* task = nullptr);
    void setSuggestedPack(const QString& name, QString version, InstanceTask* task = nullptr);
    void setSuggestedIconFromFile(const QString& path, const QString& name);
    void setSuggestedIcon(const QString& key);

    InstanceTask* extractTask();

    QString dialogTitle() override;
    QList<BasePage*> getPages() override;

    QString instName() const;
    QString instGroup() const;
    QString iconKey() const;

   public slots:
    void accept() override;
    void reject() override;

   private slots:
    void on_iconButton_clicked();
    void on_instNameTextBox_textChanged(const QString& arg1);
    void selectedPageChanged(BasePage* previous, BasePage* selected);

   private:
    Ui::NewInstanceDialog* ui = nullptr;
    PageContainer* m_container = nullptr;
    QDialogButtonBox* m_buttons = nullptr;

    QString InstIconKey;
    ImportPage* importPage = nullptr;
    std::unique_ptr<InstanceTask> creationTask;

    bool importIcon = false;
    QString importIconPath;
    QString importIconName;

    QString importVersion;

    QString m_searchTerm;

    void importIconNow();
};
