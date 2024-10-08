// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include <QMainWindow>
#include <memory>

#include "ui/pages/BasePage.h"

#include "Application.h"
#include "minecraft/auth/AccountList.h"

namespace Ui {
class AccountListPage;
}

class AuthenticateTask;

class AccountListPage : public QMainWindow, public BasePage {
    Q_OBJECT
   public:
    explicit AccountListPage(QWidget* parent = 0);
    ~AccountListPage();

    QString displayName() const override { return tr("Accounts"); }
    QIcon icon() const override
    {
        auto icon = APPLICATION->getThemedIcon("accounts");
        if (icon.isNull()) {
            icon = APPLICATION->getThemedIcon("noaccount");
        }
        return icon;
    }
    QString id() const override { return "accounts"; }
    QString helpPage() const override { return "/getting-started/adding-an-account"; }
    void retranslate() override;

   public slots:
    void on_actionAddMicrosoft_triggered();
    void on_actionAddOffline_triggered();
    void on_actionRemove_triggered();
    void on_actionRefresh_triggered();
    void on_actionSetDefault_triggered();
    void on_actionNoDefault_triggered();
    void on_actionManageSkins_triggered();

    void listChanged();

    //! Updates the states of the dialog's buttons.
    void updateButtonStates();

   protected slots:
    void ShowContextMenu(const QPoint& pos);

   private:
    void changeEvent(QEvent* event) override;
    QMenu* createPopupMenu() override;
    shared_qobject_ptr<AccountList> m_accounts;
    Ui::AccountListPage* ui;
};
