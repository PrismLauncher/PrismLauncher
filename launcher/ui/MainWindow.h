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
 *      Authors: Andrew Okin
 *               Peterix
 *               Orochimarufan <orochimarufan.x3@gmail.com>
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

#include <memory>

#include <QMainWindow>
#include <QProcess>
#include <QTimer>

#include "BaseInstance.h"
#include "minecraft/auth/MinecraftAccount.h"
#include "net/NetJob.h"
#include "updater/GoUpdate.h"

class LaunchController;
class NewsChecker;
class QToolButton;
class InstanceProxyModel;
class LabeledToolButton;
class QLabel;
class MinecraftLauncher;
class BaseProfilerFactory;
class InstanceView;
class KonamiCode;
class InstanceTask;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    class Ui;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    bool eventFilter(QObject *obj, QEvent *ev) override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent * event) override;

    void checkInstancePathForProblems();

#ifdef LAUNCHER_WITH_UPDATER
    void updatesAllowedChanged(bool allowed);
#endif

    void droppedURLs(QList<QUrl> urls);
signals:
    void isClosing();

protected:
    QMenu * createPopupMenu() override;

private slots:
    void onCatToggled(bool);

    void on_actionAbout_triggered();

    void on_actionAddInstance_triggered();

    void on_actionREDDIT_triggered();

    void on_actionMATRIX_triggered();

    void on_actionDISCORD_triggered();

    void on_actionCopyInstance_triggered();

    void on_actionChangeInstGroup_triggered();

    void on_actionChangeInstIcon_triggered();
    void on_changeIconButton_clicked(bool)
    {
        on_actionChangeInstIcon_triggered();
    }

    void on_actionViewInstanceFolder_triggered();

    void on_actionConfig_Folder_triggered();

    void on_actionViewSelectedInstFolder_triggered();

    void on_actionViewSelectedMCFolder_triggered();

    void on_actionViewSelectedModsFolder_triggered();

    void refreshInstances();

    void on_actionViewCentralModsFolder_triggered();

#ifdef LAUNCHER_WITH_UPDATER
    void checkForUpdates();
#endif

    void on_actionSettings_triggered();

    void on_actionInstanceSettings_triggered();

    void on_actionManageAccounts_triggered();

    void on_actionReportBug_triggered();

    void on_actionOpenWiki_triggered();

    void on_actionMoreNews_triggered();

    void newsButtonClicked();

    void on_actionLaunchInstance_triggered();

    void on_actionLaunchInstanceOffline_triggered();

    void on_actionKillInstance_triggered();

    void on_actionDeleteInstance_triggered();

    void deleteGroup();

    void on_actionExportInstance_triggered();

    void on_actionRenameInstance_triggered();
    void on_renameButton_clicked(bool)
    {
        on_actionRenameInstance_triggered();
    }

    void on_actionEditInstance_triggered();

    void on_actionEditInstNotes_triggered();

    void on_actionMods_triggered();

    void on_actionWorlds_triggered();

    void on_actionScreenshots_triggered();

    void taskEnd();

    /**
     * called when an icon is changed in the icon model.
     */
    void iconUpdated(QString);

    void showInstanceContextMenu(const QPoint &);

    void updateMainToolBar();

    void updateToolsMenu();

    void instanceActivated(QModelIndex);

    void instanceChanged(const QModelIndex &current, const QModelIndex &previous);

    void instanceSelectRequest(QString id);

    void instanceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

    void selectionBad();

    void startTask(Task *task);

#ifdef LAUNCHER_WITH_UPDATER
    void updateAvailable(GoUpdate::Status status);

    void updateNotAvailable();
#endif

    void defaultAccountChanged();

    void changeActiveAccount();

    void repopulateAccountsMenu();

    void updateNewsLabel();

#ifdef LAUNCHER_WITH_UPDATER
    /*!
     * Runs the DownloadTask and installs updates.
     */
    void downloadUpdates(GoUpdate::Status status);
#endif

    void konamiTriggered();

    void globalSettingsClosed();

#ifndef Q_OS_MAC
    void keyReleaseEvent(QKeyEvent *event) override;
#endif

    void refreshCurrentInstance(bool running);

private:
    void retranslateUi();

    void addInstance(QString url = QString());
    void activateInstance(InstancePtr instance);
    void setCatBackground(bool enabled);
    void updateInstanceToolIcon(QString new_icon);
    void setSelectedInstanceById(const QString &id);
    void updateStatusCenter();

    void runModalTask(Task *task);
    void instanceFromInstanceTask(InstanceTask *task);
    void finalizeInstance(InstancePtr inst);

private:
    std::unique_ptr<Ui> ui;

    // these are managed by Qt's memory management model!
    InstanceView *view = nullptr;
    InstanceProxyModel *proxymodel = nullptr;
    QToolButton *newsLabel = nullptr;
    QLabel *m_statusLeft = nullptr;
    QLabel *m_statusCenter = nullptr;
    QMenu *accountMenu = nullptr;
    QToolButton *accountMenuButton = nullptr;
    KonamiCode * secretEventFilter = nullptr;

    unique_qobject_ptr<NewsChecker> m_newsChecker;

    InstancePtr m_selectedInstance;
    QString m_currentInstIcon;

    // managed by the application object
    Task *m_versionLoadTask = nullptr;
};

