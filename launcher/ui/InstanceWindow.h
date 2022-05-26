// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
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

#include <QMainWindow>
#include <QSystemTrayIcon>

#include "LaunchController.h"
#include "launch/LaunchTask.h"

#include "ui/pages/BasePageContainer.h"

#include "QObjectPtr.h"

class QPushButton;
class PageContainer;
class InstanceWindow : public QMainWindow, public BasePageContainer
{
    Q_OBJECT

public:
    explicit InstanceWindow(InstancePtr proc, QWidget *parent = 0);
    virtual ~InstanceWindow();

    bool selectPage(QString pageId) override;
    void refreshContainer() override;

    QString instanceId();

    // save all settings and changes (prepare for launch)
    bool saveAll();

    // request closing the window (from a page)
    bool requestClose() override;

signals:
    void isClosing();

private
slots:
    void on_closeButton_clicked();
    void on_btnKillMinecraft_clicked();
    void on_btnLaunchMinecraftOffline_clicked();

    void instanceLaunchTaskChanged(shared_qobject_ptr<LaunchTask> proc);
    void runningStateChanged(bool running);
    void on_instanceStatusChanged(BaseInstance::Status, BaseInstance::Status newStatus);

protected:
    void closeEvent(QCloseEvent *) override;

private:
    void updateLaunchButtons();

private:
    shared_qobject_ptr<LaunchTask> m_proc;
    InstancePtr m_instance;
    bool m_doNotSave = false;
    PageContainer *m_container = nullptr;
    QPushButton *m_closeButton = nullptr;
    QPushButton *m_killButton = nullptr;
    QPushButton *m_launchOfflineButton = nullptr;
};
