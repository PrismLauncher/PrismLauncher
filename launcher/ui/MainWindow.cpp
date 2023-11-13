// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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

#include "Application.h"
#include "BuildConfig.h"
#include "FileSystem.h"

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QVariant>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QButtonGroup>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressDialog>
#include <QShortcut>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QWidgetAction>

#include <BaseInstance.h>
#include <DesktopServices.h>
#include <InstanceList.h>
#include <MMCZip.h>
#include <SkinUtils.h>
#include <icons/IconList.h>
#include <java/JavaInstallList.h>
#include <java/JavaUtils.h>
#include <launch/LaunchTask.h>
#include <minecraft/MinecraftInstance.h>
#include <minecraft/auth/AccountList.h>
#include <net/ApiDownload.h>
#include <net/NetJob.h>
#include <news/NewsChecker.h>
#include <tools/BaseProfiler.h>
#include <updater/ExternalUpdater.h>
#include "InstanceWindow.h"

#include "ui/dialogs/AboutDialog.h"
#include "ui/dialogs/CopyInstanceDialog.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ExportInstanceDialog.h"
#include "ui/dialogs/ExportPackDialog.h"
#include "ui/dialogs/ExportToModListDialog.h"
#include "ui/dialogs/IconPickerDialog.h"
#include "ui/dialogs/ImportResourceDialog.h"
#include "ui/dialogs/ImportResourcePackDialog.h"
#include "ui/dialogs/NewInstanceDialog.h"
#include "ui/dialogs/NewsDialog.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/dialogs/VersionSelectDialog.h"
#include "ui/instanceview/InstancesView.h"
#include "ui/themes/ITheme.h"
#include "ui/themes/ThemeManager.h"
#include "ui/widgets/LabeledToolButton.h"

#include "minecraft/PackProfile.h"
#include "minecraft/VersionFile.h"
#include "minecraft/WorldList.h"
#include "minecraft/mod/ModFolderModel.h"
#include "minecraft/mod/ResourcePackFolderModel.h"
#include "minecraft/mod/ShaderPackFolderModel.h"
#include "minecraft/mod/TexturePackFolderModel.h"
#include "minecraft/mod/tasks/LocalResourceParse.h"

#include "modplatform/ModIndex.h"
#include "modplatform/flame/FlameAPI.h"
#include "modplatform/flame/FlameModIndex.h"

#include "InstanceCopyTask.h"

#include "Json.h"

#include "MMCTime.h"

namespace {
QString profileInUseFilter(const QString& profile, bool used)
{
    if (used) {
        return QObject::tr("%1 (in use)").arg(profile);
    } else {
        return profile;
    }
}
}  // namespace

// WHY: to hold the pre-translation strings together with the T pointer, so it can be retranslated without a lot of ugly code
template <typename T>
class Translated {
   public:
    Translated() {}
    Translated(QWidget* parent) { m_contained = new T(parent); }
    void setTooltipId(const char* tooltip) { m_tooltip = tooltip; }
    void setTextId(const char* text) { m_text = text; }
    operator T*() { return m_contained; }
    T* operator->() { return m_contained; }
    void retranslate()
    {
        if (m_text) {
            QString result;
            result = QApplication::translate("MainWindow", m_text);
            if (result.contains("%1")) {
                result = result.arg(BuildConfig.LAUNCHER_DISPLAYNAME);
            }
            m_contained->setText(result);
        }
        if (m_tooltip) {
            QString result;
            result = QApplication::translate("MainWindow", m_tooltip);
            if (result.contains("%1")) {
                result = result.arg(BuildConfig.LAUNCHER_DISPLAYNAME);
            }
            m_contained->setToolTip(result);
        }
    }

   private:
    T* m_contained = nullptr;
    const char* m_text = nullptr;
    const char* m_tooltip = nullptr;
};
using TranslatedAction = Translated<QAction>;
using TranslatedToolButton = Translated<QToolButton>;

class TranslatedToolbar {
   public:
    TranslatedToolbar() {}
    TranslatedToolbar(QWidget* parent) { m_contained = new QToolBar(parent); }
    void setWindowTitleId(const char* title) { m_title = title; }
    operator QToolBar*() { return m_contained; }
    QToolBar* operator->() { return m_contained; }
    void retranslate()
    {
        if (m_title) {
            m_contained->setWindowTitle(QApplication::translate("MainWindow", m_title));
        }
    }

   private:
    QToolBar* m_contained = nullptr;
    const char* m_title = nullptr;
};

// class Ui::MainWindow {
//    public:
//     TranslatedAction actionAddInstance;
//     // TranslatedAction actionRefresh;
//     TranslatedAction actionCheckUpdate;
//     TranslatedAction actionSettings;
//     TranslatedAction actionMoreNews;
//     TranslatedAction actionManageAccounts;
//     TranslatedAction actionLaunchInstance;
//     TranslatedAction actionKillInstance;
//     TranslatedAction actionRenameInstance;
//     TranslatedAction actionChangeInstGroup;
//     TranslatedAction actionChangeInstIcon;
//     TranslatedAction actionEditInstance;
//     TranslatedAction actionViewSelectedInstFolder;
//     TranslatedAction actionDeleteInstance;
//     TranslatedAction actionCAT;
//     TranslatedAction actionTableMode;
//     TranslatedAction actionGridMode;
//     TranslatedAction actionCopyInstance;
//     TranslatedAction actionLaunchInstanceOffline;
//     TranslatedAction actionLaunchInstanceDemo;
//     TranslatedAction actionExportInstance;
//     TranslatedAction actionCreateInstanceShortcut;
//     QVector<TranslatedAction*> all_actions;

//     LabeledToolButton* renameButton = nullptr;
//     LabeledToolButton* changeIconButton = nullptr;

//     QMenu* foldersMenu = nullptr;
//     TranslatedToolButton foldersMenuButton;
//     TranslatedAction actionViewInstanceFolder;
//     TranslatedAction actionViewCentralModsFolder;

//     QMenu* editMenu = nullptr;
//     TranslatedAction actionUndoTrashInstance;

//     QMenu* helpMenu = nullptr;
//     TranslatedToolButton helpMenuButton;
//     TranslatedAction actionClearMetadata;
// #ifdef Q_OS_MAC
//     TranslatedAction actionAddToPATH;
// #endif
//     TranslatedAction actionReportBug;
//     TranslatedAction actionDISCORD;
//     TranslatedAction actionMATRIX;
//     TranslatedAction actionREDDIT;
//     TranslatedAction actionAbout;

//     TranslatedAction actionNoAccountsAdded;
//     TranslatedAction actionNoDefaultAccount;

//     TranslatedAction actionLockToolbars;

//     TranslatedAction actionChangeTheme;

//     QVector<TranslatedToolButton*> all_toolbuttons;

//     QWidget* centralWidget = nullptr;
//     QVBoxLayout* verticalLayout = nullptr;

//     QMenuBar* menuBar = nullptr;
//     QMenu* fileMenu;
//     QMenu* viewMenu;
//     QMenu* profileMenu;

//     TranslatedAction actionCloseWindow;

//     TranslatedAction actionOpenWiki;
//     TranslatedAction actionNewsMenuBar;

//     TranslatedToolbar mainToolBar;
//     TranslatedToolbar instanceToolBar;
//     TranslatedToolbar newsToolBar;
//     QVector<TranslatedToolbar*> all_toolbars;

//     void createMainToolbarActions(MainWindow* MainWindow)
//     {
//         actionAddInstance = TranslatedAction(MainWindow);
//         actionAddInstance->setObjectName(QStringLiteral("actionAddInstance"));
//         actionAddInstance->setIcon(APPLICATION->getThemedIcon("new"));
//         actionAddInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Add Instanc&e..."));
//         actionAddInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Add a new instance."));
//         actionAddInstance->setShortcut(QKeySequence::New);
//         all_actions.append(&actionAddInstance);

//         actionViewInstanceFolder = TranslatedAction(MainWindow);
//         actionViewInstanceFolder->setObjectName(QStringLiteral("actionViewInstanceFolder"));
//         actionViewInstanceFolder->setIcon(APPLICATION->getThemedIcon("viewfolder"));
//         actionViewInstanceFolder.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&View Instance Folder"));
//         actionViewInstanceFolder.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the instance folder in a file browser."));
//         all_actions.append(&actionViewInstanceFolder);

//         actionViewCentralModsFolder = TranslatedAction(MainWindow);
//         actionViewCentralModsFolder->setObjectName(QStringLiteral("actionViewCentralModsFolder"));
//         actionViewCentralModsFolder->setIcon(APPLICATION->getThemedIcon("centralmods"));
//         actionViewCentralModsFolder.setTextId(QT_TRANSLATE_NOOP("MainWindow", "View &Central Mods Folder"));
//         actionViewCentralModsFolder.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the central mods folder in a file browser."));
//         all_actions.append(&actionViewCentralModsFolder);

//         foldersMenu = new QMenu(MainWindow);
//         foldersMenu->setTitle(tr("F&olders"));
//         foldersMenu->setToolTipsVisible(true);

//         foldersMenu->addAction(actionViewInstanceFolder);
//         foldersMenu->addAction(actionViewCentralModsFolder);

//         foldersMenuButton = TranslatedToolButton(MainWindow);
//         foldersMenuButton.setTextId(QT_TRANSLATE_NOOP("MainWindow", "F&olders"));
//         foldersMenuButton.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open one of the folders shared between instances."));
//         foldersMenuButton->setMenu(foldersMenu);
//         foldersMenuButton->setPopupMode(QToolButton::InstantPopup);
//         foldersMenuButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
//         foldersMenuButton->setIcon(APPLICATION->getThemedIcon("viewfolder"));
//         foldersMenuButton->setFocusPolicy(Qt::NoFocus);
//         all_toolbuttons.append(&foldersMenuButton);

//         actionSettings = TranslatedAction(MainWindow);
//         actionSettings->setObjectName(QStringLiteral("actionSettings"));
//         actionSettings->setIcon(APPLICATION->getThemedIcon("settings"));
//         actionSettings->setMenuRole(QAction::PreferencesRole);
//         actionSettings.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Setti&ngs..."));
//         actionSettings.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Change settings."));
//         actionSettings->setShortcut(QKeySequence::Preferences);
//         all_actions.append(&actionSettings);

//         actionUndoTrashInstance = TranslatedAction(MainWindow);
//         actionUndoTrashInstance->setObjectName(QStringLiteral("actionUndoTrashInstance"));
//         actionUndoTrashInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Undo Last Instance Deletion"));
//         actionUndoTrashInstance->setEnabled(APPLICATION->instances()->trashedSomething());
//         actionUndoTrashInstance->setShortcut(QKeySequence::Undo);
//         all_actions.append(&actionUndoTrashInstance);

//         actionClearMetadata = TranslatedAction(MainWindow);
//         actionClearMetadata->setObjectName(QStringLiteral("actionClearMetadata"));
//         actionClearMetadata->setIcon(APPLICATION->getThemedIcon("refresh"));
//         actionClearMetadata.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Clear Metadata Cache"));
//         actionClearMetadata.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Clear cached metadata"));
//         all_actions.append(&actionClearMetadata);

// #ifdef Q_OS_MAC
//         actionAddToPATH = TranslatedAction(MainWindow);
//         actionAddToPATH->setObjectName(QStringLiteral("actionAddToPATH"));
//         actionAddToPATH.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Install to &PATH"));
//         actionAddToPATH.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Install a prismlauncher symlink to /usr/local/bin"));
//         all_actions.append(&actionAddToPATH);
// #endif

//         if (!BuildConfig.BUG_TRACKER_URL.isEmpty()) {
//             actionReportBug = TranslatedAction(MainWindow);
//             actionReportBug->setObjectName(QStringLiteral("actionReportBug"));
//             actionReportBug->setIcon(APPLICATION->getThemedIcon("bug"));
//             actionReportBug.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Report a &Bug..."));
//             actionReportBug.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the bug tracker to report a bug with %1."));
//             all_actions.append(&actionReportBug);
//         }

//         if (!BuildConfig.MATRIX_URL.isEmpty()) {
//             actionMATRIX = TranslatedAction(MainWindow);
//             actionMATRIX->setObjectName(QStringLiteral("actionMATRIX"));
//             actionMATRIX->setIcon(APPLICATION->getThemedIcon("matrix"));
//             actionMATRIX.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Matrix Space"));
//             actionMATRIX.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open %1 Matrix space"));
//             all_actions.append(&actionMATRIX);
//         }

//         if (!BuildConfig.DISCORD_URL.isEmpty()) {
//             actionDISCORD = TranslatedAction(MainWindow);
//             actionDISCORD->setObjectName(QStringLiteral("actionDISCORD"));
//             actionDISCORD->setIcon(APPLICATION->getThemedIcon("discord"));
//             actionDISCORD.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Discord Guild"));
//             actionDISCORD.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open %1 Discord guild."));
//             all_actions.append(&actionDISCORD);
//         }

//         if (!BuildConfig.SUBREDDIT_URL.isEmpty()) {
//             actionREDDIT = TranslatedAction(MainWindow);
//             actionREDDIT->setObjectName(QStringLiteral("actionREDDIT"));
//             actionREDDIT->setIcon(APPLICATION->getThemedIcon("reddit-alien"));
//             actionREDDIT.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Sub&reddit"));
//             actionREDDIT.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open %1 subreddit."));
//             all_actions.append(&actionREDDIT);
//         }

//         actionAbout = TranslatedAction(MainWindow);
//         actionAbout->setObjectName(QStringLiteral("actionAbout"));
//         actionAbout->setIcon(APPLICATION->getThemedIcon("about"));
//         actionAbout->setMenuRole(QAction::AboutRole);
//         actionAbout.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&About %1"));
//         actionAbout.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "View information about %1."));
//         all_actions.append(&actionAbout);

//         if (BuildConfig.UPDATER_ENABLED) {
//             actionCheckUpdate = TranslatedAction(MainWindow);
//             actionCheckUpdate->setObjectName(QStringLiteral("actionCheckUpdate"));
//             actionCheckUpdate->setIcon(APPLICATION->getThemedIcon("checkupdate"));
//             actionCheckUpdate.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Update..."));
//             actionCheckUpdate.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Check for new updates for %1."));
//             actionCheckUpdate->setMenuRole(QAction::ApplicationSpecificRole);
//             all_actions.append(&actionCheckUpdate);
//         }

//         actionCAT = TranslatedAction(MainWindow);
//         actionCAT->setObjectName(QStringLiteral("actionCAT"));
//         actionCAT->setCheckable(true);
//         actionCAT->setIcon(APPLICATION->getThemedIcon("cat"));
//         actionCAT.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Meow"));
//         actionCAT.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "It's a fluffy kitty :3"));
//         actionCAT->setPriority(QAction::LowPriority);
//         all_actions.append(&actionCAT);

//         actionTableMode = TranslatedAction(MainWindow);
//         actionTableMode->setObjectName(QStringLiteral("actionTableMode"));
//         actionTableMode->setCheckable(true);
//         actionTableMode->setIcon(APPLICATION->getThemedIcon("view-list-symbolic"));  // FIXME: add custom icon
//         actionTableMode.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Table View"));
//         actionTableMode.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Display instances in a table"));
//         actionTableMode->setPriority(QAction::LowPriority);
//         all_actions.append(&actionTableMode);

//         actionGridMode = TranslatedAction(MainWindow);
//         actionGridMode->setObjectName(QStringLiteral("actionGridMode"));
//         actionGridMode->setCheckable(true);
//         actionGridMode->setIcon(APPLICATION->getThemedIcon("view-grid-symbolic"));  // FIXME: add custom icon
//         actionGridMode.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Grid View"));
//         actionGridMode.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Display instances in a grid"));
//         actionGridMode->setPriority(QAction::LowPriority);
//         all_actions.append(&actionGridMode);

//         // profile menu and its actions
//         actionManageAccounts = TranslatedAction(MainWindow);
//         actionManageAccounts->setObjectName(QStringLiteral("actionManageAccounts"));
//         actionManageAccounts.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Manage Accounts..."));
//         // FIXME: no tooltip!
//         actionManageAccounts->setCheckable(false);
//         actionManageAccounts->setIcon(APPLICATION->getThemedIcon("accounts"));
//         all_actions.append(&actionManageAccounts);

//         actionLockToolbars = TranslatedAction(MainWindow);
//         actionLockToolbars->setObjectName(QStringLiteral("actionLockToolbars"));
//         actionLockToolbars.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Lock Toolbars"));
//         actionLockToolbars->setCheckable(true);
//         all_actions.append(&actionLockToolbars);

//         actionChangeTheme = TranslatedAction(MainWindow);
//         actionChangeTheme->setObjectName(QStringLiteral("actionChangeTheme"));
//         actionChangeTheme.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Themes"));
//         all_actions.append(&actionChangeTheme);
//     }

//     void createMainToolbar(QMainWindow* MainWindow)
//     {
//         mainToolBar = TranslatedToolbar(MainWindow);
//         mainToolBar->setVisible(menuBar->isNativeMenuBar() || !APPLICATION->settings()->get("MenuBarInsteadOfToolBar").toBool());
//         mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
//         mainToolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
//         mainToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
//         mainToolBar->setFloatable(false);
//         mainToolBar.setWindowTitleId(QT_TRANSLATE_NOOP("MainWindow", "Main Toolbar"));

//         mainToolBar->addAction(actionAddInstance);

//         mainToolBar->addSeparator();

//         QWidgetAction* foldersButtonAction = new QWidgetAction(MainWindow);
//         foldersButtonAction->setDefaultWidget(foldersMenuButton);
//         mainToolBar->addAction(foldersButtonAction);

//         mainToolBar->addAction(actionSettings);

//         helpMenu = new QMenu(MainWindow);
//         helpMenu->setToolTipsVisible(true);

//         helpMenu->addAction(actionClearMetadata);

// #ifdef Q_OS_MAC
//         helpMenu->addAction(actionAddToPATH);
// #endif

//         if (!BuildConfig.BUG_TRACKER_URL.isEmpty()) {
//             helpMenu->addAction(actionReportBug);
//         }

//         if (!BuildConfig.MATRIX_URL.isEmpty()) {
//             helpMenu->addAction(actionMATRIX);
//         }

//         if (!BuildConfig.DISCORD_URL.isEmpty()) {
//             helpMenu->addAction(actionDISCORD);
//         }

//         if (!BuildConfig.SUBREDDIT_URL.isEmpty()) {
//             helpMenu->addAction(actionREDDIT);
//         }

//         helpMenu->addAction(actionAbout);

//         helpMenuButton = TranslatedToolButton(MainWindow);
//         helpMenuButton.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Help"));
//         helpMenuButton.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Get help with %1 or Minecraft."));
//         helpMenuButton->setMenu(helpMenu);
//         helpMenuButton->setPopupMode(QToolButton::InstantPopup);
//         helpMenuButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
//         helpMenuButton->setIcon(APPLICATION->getThemedIcon("help"));
//         helpMenuButton->setFocusPolicy(Qt::NoFocus);
//         all_toolbuttons.append(&helpMenuButton);
//         QWidgetAction* helpButtonAction = new QWidgetAction(MainWindow);
//         helpButtonAction->setDefaultWidget(helpMenuButton);
//         mainToolBar->addAction(helpButtonAction);

//         if (BuildConfig.UPDATER_ENABLED) {
//             mainToolBar->addAction(actionCheckUpdate);
//         }

//         mainToolBar->addSeparator();

//         mainToolBar->addAction(actionCAT);

//         mainToolBar->addAction(actionTableMode);
//         mainToolBar->addAction(actionGridMode);

//         all_toolbars.append(&mainToolBar);
//         MainWindow->addToolBar(Qt::TopToolBarArea, mainToolBar);
//     }

//     void createMenuBar(QMainWindow* MainWindow)
//     {
//         menuBar = new QMenuBar(MainWindow);
//         menuBar->setVisible(APPLICATION->settings()->get("MenuBarInsteadOfToolBar").toBool());

//         fileMenu = menuBar->addMenu(tr("&File"));
//         // Workaround for QTBUG-94802 (https://bugreports.qt.io/browse/QTBUG-94802); also present for other menus
//         fileMenu->setSeparatorsCollapsible(false);
//         fileMenu->addAction(actionAddInstance);
//         fileMenu->addAction(actionLaunchInstance);
//         fileMenu->addAction(actionKillInstance);
//         fileMenu->addAction(actionCloseWindow);
//         fileMenu->addSeparator();
//         fileMenu->addAction(actionEditInstance);
//         fileMenu->addAction(actionChangeInstGroup);
//         fileMenu->addAction(actionViewSelectedInstFolder);
//         fileMenu->addAction(actionExportInstance);
//         fileMenu->addAction(actionDeleteInstance);
//         fileMenu->addAction(actionCopyInstance);
//         fileMenu->addSeparator();
//         fileMenu->addAction(actionSettings);

//         editMenu = menuBar->addMenu(tr("&Edit"));
//         editMenu->addAction(actionUndoTrashInstance);

//         viewMenu = menuBar->addMenu(tr("&View"));
//         viewMenu->setSeparatorsCollapsible(false);
//         viewMenu->addAction(actionChangeTheme);
//         viewMenu->addSeparator();
//         viewMenu->addAction(actionCAT);
//         viewMenu->addAction(actionTableMode);
//         viewMenu->addAction(actionGridMode);
//         viewMenu->addSeparator();

//         viewMenu->addAction(actionLockToolbars);

//         menuBar->addMenu(foldersMenu);

//         profileMenu = menuBar->addMenu(tr("&Accounts"));
//         profileMenu->setSeparatorsCollapsible(false);
//         profileMenu->addAction(actionManageAccounts);

//         helpMenu = menuBar->addMenu(tr("&Help"));
//         helpMenu->setSeparatorsCollapsible(false);
//         helpMenu->addAction(actionClearMetadata);
// #ifdef Q_OS_MAC
//         helpMenu->addAction(actionAddToPATH);
// #endif
//         helpMenu->addSeparator();
//         helpMenu->addAction(actionAbout);
//         helpMenu->addAction(actionOpenWiki);
//         helpMenu->addAction(actionNewsMenuBar);
//         helpMenu->addSeparator();
//         if (!BuildConfig.BUG_TRACKER_URL.isEmpty())
//             helpMenu->addAction(actionReportBug);
//         if (!BuildConfig.MATRIX_URL.isEmpty())
//             helpMenu->addAction(actionMATRIX);
//         if (!BuildConfig.DISCORD_URL.isEmpty())
//             helpMenu->addAction(actionDISCORD);
//         if (!BuildConfig.SUBREDDIT_URL.isEmpty())
//             helpMenu->addAction(actionREDDIT);
//         if (BuildConfig.UPDATER_ENABLED) {
//             helpMenu->addSeparator();
//             helpMenu->addAction(actionCheckUpdate);
//         }
//         MainWindow->setMenuBar(menuBar);
//     }

//     void createMenuActions(MainWindow* MainWindow)
//     {
//         actionCloseWindow = TranslatedAction(MainWindow);
//         actionCloseWindow->setObjectName(QStringLiteral("actionCloseWindow"));
//         actionCloseWindow.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Close &Window"));
//         actionCloseWindow.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Close the current window"));
//         actionCloseWindow->setShortcut(QKeySequence::Close);
//         connect(actionCloseWindow, &QAction::triggered, APPLICATION, &Application::closeCurrentWindow);
//         all_actions.append(&actionCloseWindow);

//         actionOpenWiki = TranslatedAction(MainWindow);
//         actionOpenWiki->setObjectName(QStringLiteral("actionOpenWiki"));
//         actionOpenWiki.setTextId(QT_TRANSLATE_NOOP("MainWindow", "%1 &Help"));
//         actionOpenWiki.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the %1 wiki"));
//         actionOpenWiki->setIcon(APPLICATION->getThemedIcon("help"));
//         connect(actionOpenWiki, &QAction::triggered, MainWindow, &MainWindow::on_actionOpenWiki_triggered);
//         all_actions.append(&actionOpenWiki);

//         actionNewsMenuBar = TranslatedAction(MainWindow);
//         actionNewsMenuBar->setObjectName(QStringLiteral("actionNewsMenuBar"));
//         actionNewsMenuBar.setTextId(QT_TRANSLATE_NOOP("MainWindow", "%1 &News"));
//         actionNewsMenuBar.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the %1 wiki"));
//         actionNewsMenuBar->setIcon(APPLICATION->getThemedIcon("news"));
//         connect(actionNewsMenuBar, &QAction::triggered, MainWindow, &MainWindow::on_actionMoreNews_triggered);
//         all_actions.append(&actionNewsMenuBar);
//     }

//     // "Instance actions" are actions that require an instance to be selected (i.e. "new instance" is not here)
//     // Actions that also require other conditions (e.g. a running instance) won't be changed.
//     void setInstanceActionsEnabled(bool enabled)
//     {
//         actionEditInstance->setEnabled(enabled);
//         actionChangeInstGroup->setEnabled(enabled);
//         actionViewSelectedInstFolder->setEnabled(enabled);
//         actionExportInstance->setEnabled(enabled);
//         actionDeleteInstance->setEnabled(enabled);
//         actionCopyInstance->setEnabled(enabled);
//         actionCreateInstanceShortcut->setEnabled(enabled);
//     }

//     void createNewsToolbar(QMainWindow* MainWindow)
//     {
//         newsToolBar = TranslatedToolbar(MainWindow);
//         newsToolBar->setObjectName(QStringLiteral("newsToolBar"));
//         newsToolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
//         newsToolBar->setIconSize(QSize(16, 16));
//         newsToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
//         newsToolBar->setFloatable(false);
//         newsToolBar->setWindowTitle(QT_TRANSLATE_NOOP("MainWindow", "News Toolbar"));

//         actionMoreNews = TranslatedAction(MainWindow);
//         actionMoreNews->setObjectName(QStringLiteral("actionMoreNews"));
//         actionMoreNews->setIcon(APPLICATION->getThemedIcon("news"));
//         actionMoreNews.setTextId(QT_TRANSLATE_NOOP("MainWindow", "More news..."));
//         actionMoreNews.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the development blog to read more news about %1."));
//         all_actions.append(&actionMoreNews);
//         newsToolBar->addAction(actionMoreNews);

//         all_toolbars.append(&newsToolBar);
//         MainWindow->addToolBar(Qt::BottomToolBarArea, newsToolBar);
//     }

//     void createInstanceActions(QMainWindow* MainWindow)
//     {
//         // NOTE: not added to toolbar, but used for instance context menu (right click)
//         actionChangeInstIcon = TranslatedAction(MainWindow);
//         actionChangeInstIcon->setObjectName(QStringLiteral("actionChangeInstIcon"));
//         actionChangeInstIcon->setIcon(QIcon(":/icons/instances/grass"));
//         actionChangeInstIcon->setIconVisibleInMenu(true);
//         actionChangeInstIcon.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Change Icon"));
//         actionChangeInstIcon.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Change the selected instance's icon."));
//         all_actions.append(&actionChangeInstIcon);

//         changeIconButton = new LabeledToolButton(MainWindow);
//         changeIconButton->setObjectName(QStringLiteral("changeIconButton"));
//         changeIconButton->setIcon(APPLICATION->getThemedIcon("news"));
//         changeIconButton->setToolTip(actionChangeInstIcon->toolTip());
//         changeIconButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

//         // NOTE: not added to toolbar, but used for instance context menu (right click)
//         actionRenameInstance = TranslatedAction(MainWindow);
//         actionRenameInstance->setObjectName(QStringLiteral("actionRenameInstance"));
//         actionRenameInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Rename"));
//         actionRenameInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Rename the selected instance."));
//         actionRenameInstance->setIcon(APPLICATION->getThemedIcon("rename"));
//         all_actions.append(&actionRenameInstance);

//         // the rename label is inside the rename tool button
//         renameButton = new LabeledToolButton(MainWindow);
//         renameButton->setObjectName(QStringLiteral("renameButton"));
//         renameButton->setToolTip(actionRenameInstance->toolTip());
//         renameButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

//         actionLaunchInstance = TranslatedAction(MainWindow);
//         actionLaunchInstance->setObjectName(QStringLiteral("actionLaunchInstance"));
//         actionLaunchInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Launch"));
//         actionLaunchInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Launch the selected instance."));
//         actionLaunchInstance->setIcon(APPLICATION->getThemedIcon("launch"));
//         all_actions.append(&actionLaunchInstance);

//         actionLaunchInstanceOffline = TranslatedAction(MainWindow);
//         actionLaunchInstanceOffline->setObjectName(QStringLiteral("actionLaunchInstanceOffline"));
//         actionLaunchInstanceOffline.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Launch &Offline"));
//         actionLaunchInstanceOffline.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Launch the selected instance in offline mode."));
//         all_actions.append(&actionLaunchInstanceOffline);

//         actionLaunchInstanceDemo = TranslatedAction(MainWindow);
//         actionLaunchInstanceDemo->setObjectName(QStringLiteral("actionLaunchInstanceDemo"));
//         actionLaunchInstanceDemo.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Launch &Demo"));
//         actionLaunchInstanceDemo.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Launch the selected instance in demo mode."));
//         all_actions.append(&actionLaunchInstanceDemo);

//         actionKillInstance = TranslatedAction(MainWindow);
//         actionKillInstance->setObjectName(QStringLiteral("actionKillInstance"));
//         actionKillInstance->setDisabled(true);
//         actionKillInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Kill"));
//         actionKillInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Kill the running instance"));
//         actionKillInstance->setShortcut(QKeySequence(tr("Ctrl+K")));
//         actionKillInstance->setIcon(APPLICATION->getThemedIcon("status-bad"));
//         all_actions.append(&actionKillInstance);

//         actionEditInstance = TranslatedAction(MainWindow);
//         actionEditInstance->setObjectName(QStringLiteral("actionEditInstance"));
//         actionEditInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Edit..."));
//         actionEditInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Change the instance settings, mods and versions."));
//         actionEditInstance->setShortcut(QKeySequence(tr("Ctrl+I")));
//         actionEditInstance->setIcon(APPLICATION->getThemedIcon("settings-configure"));
//         all_actions.append(&actionEditInstance);

//         actionChangeInstGroup = TranslatedAction(MainWindow);
//         actionChangeInstGroup->setObjectName(QStringLiteral("actionChangeInstGroup"));
//         actionChangeInstGroup.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Change category..."));
//         actionChangeInstGroup.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Change the selected instance's category."));
//         actionChangeInstGroup->setShortcut(QKeySequence(tr("Ctrl+G")));
//         actionChangeInstGroup->setIcon(APPLICATION->getThemedIcon("tag"));
//         all_actions.append(&actionChangeInstGroup);

//         actionViewSelectedInstFolder = TranslatedAction(MainWindow);
//         actionViewSelectedInstFolder->setObjectName(QStringLiteral("actionViewSelectedInstFolder"));
//         actionViewSelectedInstFolder.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Folder"));
//         actionViewSelectedInstFolder.setTooltipId(
//             QT_TRANSLATE_NOOP("MainWindow", "Open the selected instance's root folder in a file browser."));
//         actionViewSelectedInstFolder->setIcon(APPLICATION->getThemedIcon("viewfolder"));
//         all_actions.append(&actionViewSelectedInstFolder);

//         actionExportInstance = TranslatedAction(MainWindow);
//         actionExportInstance->setObjectName(QStringLiteral("actionExportInstance"));
//         actionExportInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "E&xport..."));
//         actionExportInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Export the selected instance as a zip file."));
//         actionExportInstance->setShortcut(QKeySequence(tr("Ctrl+E")));
//         actionExportInstance->setIcon(APPLICATION->getThemedIcon("export"));
//         all_actions.append(&actionExportInstance);

//         actionDeleteInstance = TranslatedAction(MainWindow);
//         actionDeleteInstance->setObjectName(QStringLiteral("actionDeleteInstance"));
//         actionDeleteInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Dele&te"));
//         actionDeleteInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Delete the selected instance."));
//         actionDeleteInstance->setShortcuts({ QKeySequence(tr("Backspace")), QKeySequence::Delete });
//         actionDeleteInstance->setAutoRepeat(false);
//         actionDeleteInstance->setIcon(APPLICATION->getThemedIcon("delete"));
//         all_actions.append(&actionDeleteInstance);

//         actionCopyInstance = TranslatedAction(MainWindow);
//         actionCopyInstance->setObjectName(QStringLiteral("actionCopyInstance"));
//         actionCopyInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Cop&y..."));
//         actionCopyInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Copy the selected instance."));
//         actionCopyInstance->setShortcut(QKeySequence(tr("Ctrl+D")));
//         actionCopyInstance->setIcon(APPLICATION->getThemedIcon("copy"));
//         all_actions.append(&actionCopyInstance);

//         actionCreateInstanceShortcut = TranslatedAction(MainWindow);
//         actionCreateInstanceShortcut->setObjectName(QStringLiteral("actionCreateInstanceShortcut"));
//         actionCreateInstanceShortcut.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Create Shortcut"));
//         actionCreateInstanceShortcut.setTooltipId(
//             QT_TRANSLATE_NOOP("MainWindow", "Creates a shortcut on your desktop to launch the selected instance."));
//         // actionCreateInstanceShortcut->setShortcut(QKeySequence(tr("Ctrl+D")));     // TODO
//         //  FIXME missing on Legacy, Flat and Flat (White)
//         actionCreateInstanceShortcut->setIcon(APPLICATION->getThemedIcon("shortcut"));
//         all_actions.append(&actionCreateInstanceShortcut);

//         setInstanceActionsEnabled(false);
//     }

//     void createInstanceToolbar(QMainWindow* MainWindow)
//     {
//         instanceToolBar = TranslatedToolbar(MainWindow);
//         instanceToolBar->setObjectName(QStringLiteral("instanceToolBar"));
//         // disabled until we have an instance selected
//         instanceToolBar->setEnabled(false);
//         // Qt doesn't like vertical moving toolbars, so we have to force them...
//         // See https://github.com/PolyMC/PolyMC/issues/493
//         connect(instanceToolBar, &QToolBar::orientationChanged, [=](Qt::Orientation) { instanceToolBar->setOrientation(Qt::Vertical); });
//         instanceToolBar->setAllowedAreas(Qt::LeftToolBarArea | Qt::RightToolBarArea);
//         instanceToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
//         instanceToolBar->setIconSize(QSize(16, 16));

//         instanceToolBar->setFloatable(false);
//         instanceToolBar->setWindowTitle(QT_TRANSLATE_NOOP("MainWindow", "Instance Toolbar"));

//         instanceToolBar->addWidget(changeIconButton);
//         instanceToolBar->addWidget(renameButton);

//         instanceToolBar->addSeparator();

//         instanceToolBar->addAction(actionLaunchInstance);
//         instanceToolBar->addAction(actionKillInstance);

//         instanceToolBar->addSeparator();

//         instanceToolBar->addAction(actionEditInstance);
//         instanceToolBar->addAction(actionChangeInstGroup);

//         instanceToolBar->addAction(actionViewSelectedInstFolder);

//         instanceToolBar->addAction(actionExportInstance);
//         instanceToolBar->addAction(actionCopyInstance);
//         instanceToolBar->addAction(actionDeleteInstance);

//         instanceToolBar->addAction(actionCreateInstanceShortcut);  // TODO find better position for this

//         QLayout* lay = instanceToolBar->layout();
//         for (int i = 0; i < lay->count(); i++) {
//             QLayoutItem* item = lay->itemAt(i);
//             if (item->widget()->metaObject()->className() == QString("QToolButton")) {
//                 item->setAlignment(Qt::AlignLeft);
//             }
//         }

//         all_toolbars.append(&instanceToolBar);
//         MainWindow->addToolBar(Qt::RightToolBarArea, instanceToolBar);
//     }

//     void setupUi(MainWindow* MainWindow)
//     {
//         if (MainWindow->objectName().isEmpty()) {
//             MainWindow->setObjectName(QStringLiteral("MainWindow"));
//         }
//         MainWindow->resize(800, 600);
//         MainWindow->setWindowIcon(APPLICATION->getThemedIcon("logo"));
//         MainWindow->setWindowTitle(APPLICATION->applicationDisplayName());
// #ifndef QT_NO_ACCESSIBILITY
//         MainWindow->setAccessibleName(BuildConfig.LAUNCHER_DISPLAYNAME);
// #endif

//         createMainToolbarActions(MainWindow);
//         createMenuActions(MainWindow);
//         createInstanceActions(MainWindow);

//         createMenuBar(MainWindow);

//         createMainToolbar(MainWindow);

//         centralWidget = new QWidget(MainWindow);
//         centralWidget->setObjectName(QStringLiteral("centralWidget"));
//         verticalLayout = new QVBoxLayout(centralWidget);
//         verticalLayout->setSpacing(0);
//         verticalLayout->setObjectName(QStringLiteral("horizontalLayout"));
//         verticalLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
//         verticalLayout->setContentsMargins(0, 0, 0, 0);
//         MainWindow->setCentralWidget(centralWidget);

//         createNewsToolbar(MainWindow);
//         createInstanceToolbar(MainWindow);

//         MainWindow->updateToolsMenu();
//         MainWindow->updateThemeMenu();

//         retranslateUi(MainWindow);

//         QMetaObject::connectSlotsByName(MainWindow);
//     }  // setupUi

//     void retranslateUi(MainWindow* MainWindow)
//     {
//         // all the actions
//         for (auto* item : all_actions) {
//             item->retranslate();
//         }
//         for (auto* item : all_toolbars) {
//             item->retranslate();
//         }
//         for (auto* item : all_toolbuttons) {
//             item->retranslate();
//         }
//         // submenu buttons
//         foldersMenuButton->setText(tr("Folders"));
//         helpMenuButton->setText(tr("Help"));
//     }  // retranslateUi
// };

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowIcon(APPLICATION->getThemedIcon("logo"));
    setWindowTitle(APPLICATION->applicationDisplayName());
#ifndef QT_NO_ACCESSIBILITY
    setAccessibleName(BuildConfig.LAUNCHER_DISPLAYNAME);
#endif

    // instance toolbar stuff
    {
        // Qt doesn't like vertical moving toolbars, so we have to force them...
        // See https://github.com/PolyMC/PolyMC/issues/493
        connect(ui->instanceToolBar, &QToolBar::orientationChanged,
                [=](Qt::Orientation) { ui->instanceToolBar->setOrientation(Qt::Vertical); });

        // if you try to add a widget to a toolbar in a .ui file
        // qt designer will delete it when you save the file >:(
        changeIconButton = new LabeledToolButton(this);
        changeIconButton->setObjectName(QStringLiteral("changeIconButton"));
        changeIconButton->setIcon(APPLICATION->getThemedIcon("news"));
        changeIconButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        connect(changeIconButton, &QToolButton::clicked, this, &MainWindow::on_actionChangeInstIcon_triggered);
        ui->instanceToolBar->insertWidgetBefore(ui->actionLaunchInstance, changeIconButton);

        renameButton = new LabeledToolButton(this);
        renameButton->setObjectName(QStringLiteral("renameButton"));
        renameButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        connect(renameButton, &QToolButton::clicked, this, &MainWindow::on_actionRenameInstance_triggered);
        ui->instanceToolBar->insertWidgetBefore(ui->actionLaunchInstance, renameButton);

        ui->instanceToolBar->insertSeparator(ui->actionLaunchInstance);

        // restore the instance toolbar settings
        auto const setting_name = QString("WideBarVisibility_%1").arg(ui->instanceToolBar->objectName());
        if (!APPLICATION->settings()->contains(setting_name))
            instanceToolbarSetting = APPLICATION->settings()->registerSetting(setting_name);
        else
            instanceToolbarSetting = APPLICATION->settings()->getSetting(setting_name);

        ui->instanceToolBar->setVisibilityState(instanceToolbarSetting->get().toByteArray());

        ui->instanceToolBar->addContextMenuAction(ui->newsToolBar->toggleViewAction());
        ui->instanceToolBar->addContextMenuAction(ui->instanceToolBar->toggleViewAction());
        ui->instanceToolBar->addContextMenuAction(ui->actionLockToolbars);
    }

    // set the menu for the folders help, accounts, and export tool buttons
    {
        auto foldersMenuButton = dynamic_cast<QToolButton*>(ui->mainToolBar->widgetForAction(ui->actionFoldersButton));
        ui->actionFoldersButton->setMenu(ui->foldersMenu);
        foldersMenuButton->setPopupMode(QToolButton::InstantPopup);

        helpMenuButton = dynamic_cast<QToolButton*>(ui->mainToolBar->widgetForAction(ui->actionHelpButton));
        ui->actionHelpButton->setMenu(new QMenu(this));
        ui->actionHelpButton->menu()->addActions(ui->helpMenu->actions());
        ui->actionHelpButton->menu()->removeAction(ui->actionCheckUpdate);
        helpMenuButton->setPopupMode(QToolButton::InstantPopup);

        auto accountMenuButton = dynamic_cast<QToolButton*>(ui->mainToolBar->widgetForAction(ui->actionAccountsButton));
        accountMenuButton->setPopupMode(QToolButton::InstantPopup);

        auto exportInstanceMenu = new QMenu(this);
        exportInstanceMenu->addAction(ui->actionExportInstanceZip);
        exportInstanceMenu->addAction(ui->actionExportInstanceMrPack);
        exportInstanceMenu->addAction(ui->actionExportInstanceFlamePack);
        exportInstanceMenu->addAction(ui->actionExportInstanceToModList);
        ui->actionExportInstance->setMenu(exportInstanceMenu);
    }

    // hide, disable and show stuff
    {
        ui->actionReportBug->setVisible(!BuildConfig.BUG_TRACKER_URL.isEmpty());
        ui->actionMATRIX->setVisible(!BuildConfig.MATRIX_URL.isEmpty());
        ui->actionDISCORD->setVisible(!BuildConfig.DISCORD_URL.isEmpty());
        ui->actionREDDIT->setVisible(!BuildConfig.SUBREDDIT_URL.isEmpty());

        ui->actionCheckUpdate->setVisible(APPLICATION->updaterEnabled());

#ifndef Q_OS_MAC
        ui->actionAddToPATH->setVisible(false);
#endif

        // disabled until we have an instance selected
        ui->instanceToolBar->setEnabled(false);
        setInstanceActionsEnabled(false);

        // add a close button at the end of the main toolbar when running on gamescope / steam deck
        // FIXME: detect if we don't have server side decorations instead
        if (qgetenv("XDG_CURRENT_DESKTOP") == "gamescope") {
            ui->mainToolBar->addAction(ui->actionCloseWindow);
        }
    }

    // add the toolbar toggles to the view menu
    ui->viewMenu->addAction(ui->instanceToolBar->toggleViewAction());
    ui->viewMenu->addAction(ui->newsToolBar->toggleViewAction());

    updateThemeMenu();
    updateMainToolBar();
    // OSX magic.
    setUnifiedTitleAndToolBarOnMac(true);

    // Global shortcuts
    {
        // you can't set QKeySequence::StandardKey shortcuts in qt designer >:(
        ui->actionAddInstance->setShortcut(QKeySequence::New);
        ui->actionSettings->setShortcut(QKeySequence::Preferences);
        ui->actionUndoTrashInstance->setShortcut(QKeySequence::Undo);
        ui->actionDeleteInstance->setShortcuts({ QKeySequence(tr("Backspace")), QKeySequence::Delete });
        ui->actionCloseWindow->setShortcut(QKeySequence::Close);
        connect(ui->actionCloseWindow, &QAction::triggered, APPLICATION, &Application::closeCurrentWindow);

        // FIXME: This is kinda weird. and bad. We need some kind of managed shutdown.
        auto q = new QShortcut(QKeySequence::Quit, this);
        connect(q, &QShortcut::activated, APPLICATION, &Application::quit);
    }

    // Add the news label to the news toolbar.
    {
        m_newsChecker.reset(new NewsChecker(APPLICATION->network(), BuildConfig.NEWS_RSS_URL));
        newsLabel = new QToolButton();
        newsLabel->setIcon(APPLICATION->getThemedIcon("news"));
        newsLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        newsLabel->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        newsLabel->setFocusPolicy(Qt::NoFocus);
        ui->newsToolBar->insertWidget(ui->actionMoreNews, newsLabel);

        QObject::connect(newsLabel, &QAbstractButton::clicked, this, &MainWindow::newsButtonClicked);
        QObject::connect(m_newsChecker.get(), &NewsChecker::newsLoaded, this, &MainWindow::updateNewsLabel);
        updateNewsLabel();
    }

    // Create instance list filter box
    {
        filterView = new QLineEdit(ui->centralWidget);
        filterView->setPlaceholderText(tr("Search and filter..."));

        ui->verticalLayout->addWidget(filterView);
    }

    // Create the instance list widget
    {
        view = new InstancesView(ui->centralWidget, APPLICATION->instances().get());

        connect(view, &InstancesView::showContextMenu, this, &MainWindow::showInstanceContextMenu);
        connect(view, &InstancesView::refreshInstances, this, &MainWindow::refreshInstances);
        connect(filterView, &QLineEdit::textEdited, view, &InstancesView::setFilterQuery);

        ui->verticalLayout->addWidget(view);
    }
    // The cat background
    {
        // set the cat action priority here so you can still see the action in qt designer
        ui->actionCAT->setPriority(QAction::LowPriority);
        bool cat_enable = APPLICATION->settings()->get("TheCat").toBool();
        ui->actionCAT->setChecked(cat_enable);
        // NOTE: calling the operator like that is an ugly hack to appease ancient gcc...
        connect(ui->actionCAT, &QAction::toggled, this, &MainWindow::onCatToggled);
        connect(APPLICATION, &Application::currentCatChanged, this, &MainWindow::onCatChanged);
        view->setCatDisplayed(cat_enable);
    }
    // Table/Grid mode
    {
        int displayMode = APPLICATION->settings()->get("InstanceDisplayMode").toInt();
        ui->actionTableMode->setChecked(false);
        ui->actionGridMode->setChecked(false);
        if (displayMode == InstancesView::TableMode) {
            ui->actionTableMode->setChecked(true);
        } else {
            ui->actionGridMode->setChecked(true);
        }
        view->switchDisplayMode(displayMode == InstancesView::TableMode ? InstancesView::TableMode : InstancesView::GridMode);

        connect(ui->actionTableMode, &QAction::triggered, this, [this](bool state) {
            if (!state) {
                ui->actionTableMode->setChecked(true);
            }
            ui->actionGridMode->setChecked(false);
            view->switchDisplayMode(InstancesView::TableMode);
            APPLICATION->settings()->set("InstanceDisplayMode", InstancesView::TableMode);
        });

        connect(ui->actionGridMode, &QAction::triggered, this, [this](bool state) {
            if (!state) {
                ui->actionGridMode->setChecked(true);
            }
            ui->actionTableMode->setChecked(false);
            view->switchDisplayMode(InstancesView::GridMode);
            APPLICATION->settings()->set("InstanceDisplayMode", InstancesView::GridMode);
        });
    }

    // Lock toolbars
    {
        bool toolbarsLocked = APPLICATION->settings()->get("ToolbarsLocked").toBool();
        ui->actionLockToolbars->setChecked(toolbarsLocked);
        connect(ui->actionLockToolbars, &QAction::toggled, this, &MainWindow::lockToolbars);
        lockToolbars(toolbarsLocked);
    }
    // start instance when double-clicked
    connect(view, &InstancesView::instanceActivated, this, &MainWindow::instanceActivated);

    // track the selection -- update the instance toolbar
    connect(view, &InstancesView::currentInstanceChanged, this, &MainWindow::instanceChanged);

    // track icon changes and update the toolbar!
    connect(APPLICATION->icons().get(), &IconList::iconUpdated, this, &MainWindow::iconUpdated);

    // model reset -> selection is invalid. All the instance pointers are wrong.
    connect(APPLICATION->instances().get(), &InstanceList::dataIsInvalid, this, &MainWindow::selectionBad);

    // handle newly added instances
    connect(APPLICATION->instances().get(), &InstanceList::instanceSelectRequest, this, &MainWindow::instanceSelectRequest);

    // When the global settings page closes, we want to know about it and update our state
    connect(APPLICATION, &Application::globalSettingsClosed, this, &MainWindow::globalSettingsClosed);

    // Add "manage accounts" button, right align
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->mainToolBar->insertWidget(ui->actionAccountsButton, spacer);

    // Use undocumented property... https://stackoverflow.com/questions/7121718/create-a-scrollbar-in-a-submenu-qt
    ui->accountsMenu->setStyleSheet("QMenu { menu-scrollable: 1; }");

    repopulateAccountsMenu();

    // Update the menu when the active account changes.
    // Shouldn't have to use lambdas here like this, but if I don't, the compiler throws a fit.
    // Template hell sucks...
    connect(APPLICATION->accounts().get(), &AccountList::defaultAccountChanged, [this] { defaultAccountChanged(); });
    connect(APPLICATION->accounts().get(), &AccountList::listChanged, [this] { defaultAccountChanged(); });

    // Show initial account
    defaultAccountChanged();

    // TODO: refresh accounts here?
    // auto accounts = APPLICATION->accounts();

    // load the news
    {
        m_newsChecker->reloadNews();
        updateNewsLabel();
    }

    if (APPLICATION->updaterEnabled()) {
        bool updatesAllowed = APPLICATION->updatesAreAllowed();
        updatesAllowedChanged(updatesAllowed);

        connect(ui->actionCheckUpdate, &QAction::triggered, this, &MainWindow::checkForUpdates);

        // set up the updater object.
        auto updater = APPLICATION->updater();

        if (updater) {
            connect(updater.get(), &ExternalUpdater::canCheckForUpdatesChanged, this, &MainWindow::updatesAllowedChanged);
        }
    }

    connect(ui->actionUndoTrashInstance, &QAction::triggered, this, &MainWindow::undoTrashInstance);

    setSelectedInstanceById(APPLICATION->settings()->get("SelectedInstance").toString());

    // removing this looks stupid
    view->setFocus();

    retranslateUi();
}

// macOS always has a native menu bar, so these fixes are not applicable
// Other systems may or may not have a native menu bar (most do not - it seems like only Ubuntu Unity does)
#ifndef Q_OS_MAC
void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Alt && !APPLICATION->settings()->get("MenuBarInsteadOfToolBar").toBool())
        ui->menuBar->setVisible(!ui->menuBar->isVisible());
    else
        QMainWindow::keyReleaseEvent(event);
}
#endif

void MainWindow::retranslateUi()
{
    ui->retranslateUi(this);

    MinecraftAccountPtr defaultAccount = APPLICATION->accounts()->defaultAccount();
    if (defaultAccount) {
        auto profileLabel = profileInUseFilter(defaultAccount->profileName(), defaultAccount->isInUse());
        ui->actionAccountsButton->setText(profileLabel);
    }

    changeIconButton->setToolTip(ui->actionChangeInstIcon->toolTip());
    renameButton->setToolTip(ui->actionRenameInstance->toolTip());

    // replace the %1 with the launcher display name in some actions
    if (helpMenuButton->toolTip().contains("%1"))
        helpMenuButton->setToolTip(helpMenuButton->toolTip().arg(BuildConfig.LAUNCHER_DISPLAYNAME));

    for (auto action : ui->helpMenu->actions()) {
        if (action->text().contains("%1"))
            action->setText(action->text().arg(BuildConfig.LAUNCHER_DISPLAYNAME));
        if (action->toolTip().contains("%1"))
            action->setToolTip(action->toolTip().arg(BuildConfig.LAUNCHER_DISPLAYNAME));
    }
}

MainWindow::~MainWindow() {}

QMenu* MainWindow::createPopupMenu()
{
    QMenu* filteredMenu = QMainWindow::createPopupMenu();
    filteredMenu->removeAction(ui->mainToolBar->toggleViewAction());

    filteredMenu->addAction(ui->actionLockToolbars);

    return filteredMenu;
}
void MainWindow::lockToolbars(bool state)
{
    ui->mainToolBar->setMovable(!state);
    ui->instanceToolBar->setMovable(!state);
    ui->newsToolBar->setMovable(!state);
    APPLICATION->settings()->set("ToolbarsLocked", state);
}

void MainWindow::showInstanceContextMenu(const QPoint& pos, InstancePtr inst)
{
    QList<QAction*> actions;

    QAction* actionSep = new QAction("", this);
    actionSep->setSeparator(true);

    actions = ui->instanceToolBar->actions();

    // replace the change icon widget with an actual action
    actions.replace(0, ui->actionChangeInstIcon);

    // replace the rename widget with an actual action
    actions.replace(1, ui->actionRenameInstance);

    // add header
    actions.prepend(actionSep);
    QAction* actionVoid = new QAction(m_selectedInstance->name(), this);
    actionVoid->setEnabled(false);
    actions.prepend(actionVoid);

    //     QAction* actionVoid = new QAction(group.isNull() ? BuildConfig.LAUNCHER_DISPLAYNAME : group, this);
    //     actionVoid->setEnabled(false);

    //     QAction* actionCreateInstance = new QAction(tr("&Create instance"), this);
    //     actionCreateInstance->setToolTip(ui->actionAddInstance->toolTip());
    //     if (!group.isNull()) {
    //         QVariantMap instance_action_data;
    //         instance_action_data["group"] = group;
    //         actionCreateInstance->setData(instance_action_data);
    //     }

    //     connect(actionCreateInstance, SIGNAL(triggered(bool)), SLOT(on_actionAddInstance_triggered()));

    //     actions.prepend(actionSep);
    //     actions.prepend(actionVoid);
    //     actions.append(actionCreateInstance);
    //     if (!group.isNull()) {
    //         QAction* actionDeleteGroup = new QAction(tr("&Delete group"), this);
    //         connect(actionDeleteGroup, &QAction::triggered, this, [this, group] { deleteGroup(group); });
    //         actions.append(actionDeleteGroup);

    //         QAction* actionRenameGroup = new QAction(tr("&Rename group"), this);
    //         connect(actionRenameGroup, &QAction::triggered, this, [this, group] { renameGroup(group); });
    //         actions.append(actionRenameGroup);
    //     }
    // }
    QMenu myMenu;
    myMenu.addActions(actions);
    /*
    if (onInstance)
        myMenu.setEnabled(m_selectedInstance->canLaunch());
    */
    myMenu.exec(view->mapToGlobal(pos));
}

void MainWindow::updateMainToolBar()
{
    ui->menuBar->setVisible(APPLICATION->settings()->get("MenuBarInsteadOfToolBar").toBool());
    ui->mainToolBar->setVisible(ui->menuBar->isNativeMenuBar() || !APPLICATION->settings()->get("MenuBarInsteadOfToolBar").toBool());
}

void MainWindow::updateLaunchButton()
{
    QMenu* launchMenu = ui->actionLaunchInstance->menu();
    if (launchMenu)
        launchMenu->clear();
    else
        launchMenu = new QMenu(this);
    if (m_selectedInstance)
        m_selectedInstance->populateLaunchMenu(launchMenu);
    ui->actionLaunchInstance->setMenu(launchMenu);
}

void MainWindow::updateThemeMenu()
{
    QMenu* themeMenu = ui->actionChangeTheme->menu();

    if (themeMenu) {
        themeMenu->clear();
    } else {
        themeMenu = new QMenu(this);
    }

    auto themes = APPLICATION->themeManager()->getValidApplicationThemes();

    QActionGroup* themesGroup = new QActionGroup(this);

    for (auto* theme : themes) {
        QAction* themeAction = themeMenu->addAction(theme->name());

        themeAction->setCheckable(true);
        if (APPLICATION->settings()->get("ApplicationTheme").toString() == theme->id()) {
            themeAction->setChecked(true);
        }
        themeAction->setActionGroup(themesGroup);

        connect(themeAction, &QAction::triggered, [theme]() {
            APPLICATION->themeManager()->setApplicationTheme(theme->id());
            APPLICATION->settings()->set("ApplicationTheme", theme->id());
        });
    }

    ui->actionChangeTheme->setMenu(themeMenu);
}

void MainWindow::repopulateAccountsMenu()
{
    ui->accountsMenu->clear();

    // NOTE: this is done so the accounts button text is not set to the accounts menu title
    QMenu* accountsButtonMenu = ui->actionAccountsButton->menu();
    if (accountsButtonMenu) {
        accountsButtonMenu->clear();
    } else {
        accountsButtonMenu = new QMenu(this);
        ui->actionAccountsButton->setMenu(accountsButtonMenu);
    }

    auto accounts = APPLICATION->accounts();
    MinecraftAccountPtr defaultAccount = accounts->defaultAccount();

    QString active_profileId = "";
    if (defaultAccount) {
        // this can be called before accountMenuButton exists
        if (ui->actionAccountsButton) {
            auto profileLabel = profileInUseFilter(defaultAccount->profileName(), defaultAccount->isInUse());
            ui->actionAccountsButton->setText(profileLabel);
        }
    }

    QActionGroup* accountsGroup = new QActionGroup(this);

    if (accounts->count() <= 0) {
        ui->actionNoAccountsAdded->setEnabled(false);
        ui->accountsMenu->addAction(ui->actionNoAccountsAdded);
    } else {
        // TODO: Nicer way to iterate?
        for (int i = 0; i < accounts->count(); i++) {
            MinecraftAccountPtr account = accounts->at(i);
            auto profileLabel = profileInUseFilter(account->profileName(), account->isInUse());
            QAction* action = new QAction(profileLabel, this);
            action->setData(i);
            action->setCheckable(true);
            action->setActionGroup(accountsGroup);
            if (defaultAccount == account) {
                action->setChecked(true);
            }

            auto face = account->getFace();
            if (!face.isNull()) {
                action->setIcon(face);
            } else {
                action->setIcon(APPLICATION->getThemedIcon("noaccount"));
            }

            const int highestNumberKey = 9;
            if (i < highestNumberKey) {
                action->setShortcut(QKeySequence(tr("Ctrl+%1").arg(i + 1)));
            }

            ui->accountsMenu->addAction(action);
            connect(action, SIGNAL(triggered(bool)), SLOT(changeActiveAccount()));
        }
    }

    ui->accountsMenu->addSeparator();

    ui->actionNoDefaultAccount->setData(-1);
    ui->actionNoDefaultAccount->setChecked(!defaultAccount);
    ui->actionNoDefaultAccount->setActionGroup(accountsGroup);

    ui->accountsMenu->addAction(ui->actionNoDefaultAccount);

    connect(ui->actionNoDefaultAccount, SIGNAL(triggered(bool)), SLOT(changeActiveAccount()));

    ui->accountsMenu->addSeparator();
    ui->accountsMenu->addAction(ui->actionManageAccounts);

    accountsButtonMenu->addActions(ui->accountsMenu->actions());
}

void MainWindow::updatesAllowedChanged(bool allowed)
{
    if (!APPLICATION->updaterEnabled()) {
        return;
    }
    ui->actionCheckUpdate->setEnabled(allowed);
}

/*
 * Assumes the sender is a QAction
 */
void MainWindow::changeActiveAccount()
{
    QAction* sAction = (QAction*)sender();

    // Profile's associated Mojang username
    if (sAction->data().type() != QVariant::Type::Int)
        return;

    QVariant action_data = sAction->data();
    bool valid = false;
    int index = action_data.toInt(&valid);
    if (!valid) {
        index = -1;
    }
    auto accounts = APPLICATION->accounts();
    accounts->setDefaultAccount(index == -1 ? nullptr : accounts->at(index));
    defaultAccountChanged();
}

void MainWindow::defaultAccountChanged()
{
    repopulateAccountsMenu();

    MinecraftAccountPtr account = APPLICATION->accounts()->defaultAccount();

    // FIXME: this needs adjustment for MSA
    if (account && account->profileName() != "") {
        auto profileLabel = profileInUseFilter(account->profileName(), account->isInUse());
        ui->actionAccountsButton->setText(profileLabel);
        auto face = account->getFace();
        if (face.isNull()) {
            ui->actionAccountsButton->setIcon(APPLICATION->getThemedIcon("noaccount"));
        } else {
            ui->actionAccountsButton->setIcon(face);
        }
        return;
    }

    // Set the icon to the "no account" icon.
    ui->actionAccountsButton->setIcon(APPLICATION->getThemedIcon("noaccount"));
    ui->actionAccountsButton->setText(tr("Accounts"));
}

bool MainWindow::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj == view) {
        if (ev->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(ev);
            switch (keyEvent->key()) {
                    /*
                case Qt::Key_Enter:
                case Qt::Key_Return:
                    activateInstance(m_selectedInstance);
                    return true;
                    */
                case Qt::Key_Delete:
                    on_actionDeleteInstance_triggered();
                    return true;
                case Qt::Key_F5:
                    refreshInstances();
                    return true;
                default:
                    break;
            }
        }
    }
    return QMainWindow::eventFilter(obj, ev);
}

void MainWindow::updateNewsLabel()
{
    if (m_newsChecker->isLoadingNews()) {
        newsLabel->setText(tr("Loading news..."));
        newsLabel->setEnabled(false);
        ui->actionMoreNews->setVisible(false);
    } else {
        QList<NewsEntryPtr> entries = m_newsChecker->getNewsEntries();
        if (entries.length() > 0) {
            newsLabel->setText(entries[0]->title);
            newsLabel->setEnabled(true);
            ui->actionMoreNews->setVisible(true);
        } else {
            newsLabel->setText(tr("No news available."));
            newsLabel->setEnabled(false);
            ui->actionMoreNews->setVisible(false);
        }
    }
}

QList<int> stringToIntList(const QString& string)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QStringList split = string.split(',', Qt::SkipEmptyParts);
#else
    QStringList split = string.split(',', QString::SkipEmptyParts);
#endif
    QList<int> out;
    for (int i = 0; i < split.size(); ++i) {
        out.append(split.at(i).toInt());
    }
    return out;
}
QString intListToString(const QList<int>& list)
{
    QStringList slist;
    for (int i = 0; i < list.size(); ++i) {
        slist.append(QString::number(list.at(i)));
    }
    return slist.join(',');
}

void MainWindow::onCatToggled(bool state)
{
    view->setCatDisplayed(state);
    APPLICATION->settings()->set("TheCat", state);
}

void MainWindow::runModalTask(Task* task)
{
    connect(task, &Task::failed,
            [this](QString reason) { CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show(); });
    connect(task, &Task::succeeded, [this, task]() {
        QStringList warnings = task->warnings();
        if (warnings.count()) {
            CustomMessageBox::selectable(this, tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->show();
        }
    });
    connect(task, &Task::aborted, [this] {
        CustomMessageBox::selectable(this, tr("Task aborted"), tr("The task has been aborted by the user."), QMessageBox::Information)
            ->show();
    });
    ProgressDialog loadDialog(this);
    loadDialog.setSkipButton(true, tr("Abort"));
    loadDialog.execWithTask(task);
}

void MainWindow::instanceFromInstanceTask(InstanceTask* rawTask)
{
    unique_qobject_ptr<Task> task(APPLICATION->instances()->wrapInstanceTask(rawTask));
    runModalTask(task.get());
}

void MainWindow::on_actionCopyInstance_triggered()
{
    if (!m_selectedInstance)
        return;

    CopyInstanceDialog copyInstDlg(m_selectedInstance, this);
    if (!copyInstDlg.exec())
        return;

    auto copyTask = new InstanceCopyTask(m_selectedInstance, copyInstDlg.getChosenOptions());
    copyTask->setName(copyInstDlg.instName());
    copyTask->setGroup(copyInstDlg.instGroup());
    copyTask->setIcon(copyInstDlg.iconKey());
    unique_qobject_ptr<Task> task(APPLICATION->instances()->wrapInstanceTask(copyTask));
    runModalTask(task.get());
}

void MainWindow::finalizeInstance(InstancePtr inst)
{
    setSelectedInstanceById(inst->id());
    if (APPLICATION->accounts()->anyAccountIsValid()) {
        ProgressDialog loadDialog(this);
        auto update = inst->createUpdateTask(Net::Mode::Online);
        connect(update.get(), &Task::failed, [this](QString reason) {
            QString error = QString("Instance load failed: %1").arg(reason);
            CustomMessageBox::selectable(this, tr("Error"), error, QMessageBox::Warning)->show();
        });
        if (update) {
            loadDialog.setSkipButton(true, tr("Abort"));
            loadDialog.execWithTask(update.get());
        }
    } else {
        CustomMessageBox::selectable(this, tr("Error"),
                                     tr("The launcher cannot download Minecraft or update instances unless you have at least "
                                        "one account added.\nPlease add a Microsoft account."),
                                     QMessageBox::Warning)
            ->show();
    }
}

void MainWindow::addInstance(const QString& url, const QMap<QString, QString>& extra_info)
{
    QString groupName;
    do {
        QObject* obj = sender();
        if (!obj)
            break;
        QAction* action = qobject_cast<QAction*>(obj);
        if (!action)
            break;
        auto map = action->data().toMap();
        if (!map.contains("group"))
            break;
        groupName = map["group"].toString();
    } while (0);

    if (groupName.isEmpty()) {
        groupName = APPLICATION->settings()->get("LastUsedGroupForNewInstance").toString();
    }

    NewInstanceDialog newInstDlg(groupName, url, extra_info, this);
    if (!newInstDlg.exec())
        return;

    APPLICATION->settings()->set("LastUsedGroupForNewInstance", newInstDlg.instGroup());

    InstanceTask* creationTask = newInstDlg.extractTask();
    if (creationTask) {
        instanceFromInstanceTask(creationTask);
    }
}

void MainWindow::on_actionAddInstance_triggered()
{
    addInstance();
}

void MainWindow::processURLs(QList<QUrl> urls)
{
    // NOTE: This loop only processes one dropped file!
    for (auto& url : urls) {
        qDebug() << "Processing" << url;

        // The isLocalFile() check below doesn't work as intended without an explicit scheme.
        if (url.scheme().isEmpty())
            url.setScheme("file");

        ModPlatform::IndexedVersion version;
        QMap<QString, QString> extra_info;
        QUrl local_url;
        if (!url.isLocalFile()) {  // download the remote resource and identify
            QUrl dl_url;
            if (url.scheme() == "curseforge") {
                // need to find the download link for the modpack / resource
                // format of url curseforge://install?addonId=IDHERE&fileId=IDHERE
                QUrlQuery query(url);

                if (query.allQueryItemValues("addonId").isEmpty() || query.allQueryItemValues("fileId").isEmpty()) {
                    qDebug() << "Invalid curseforge link:" << url;
                    continue;
                }

                auto addonId = query.allQueryItemValues("addonId")[0];
                auto fileId = query.allQueryItemValues("fileId")[0];

                extra_info.insert("pack_id", addonId);
                extra_info.insert("pack_version_id", fileId);

                auto array = std::make_shared<QByteArray>();

                auto api = FlameAPI();
                auto job = api.getFile(addonId, fileId, array);

                connect(job.get(), &Task::failed, this,
                        [this](QString reason) { CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show(); });
                connect(job.get(), &Task::succeeded, this, [this, array, addonId, fileId, &dl_url, &version] {
                    qDebug() << "Returned CFURL Json:\n" << array->toStdString().c_str();
                    auto doc = Json::requireDocument(*array);
                    auto data = Json::ensureObject(Json::ensureObject(doc.object()), "data");
                    // No way to find out if it's a mod or a modpack before here
                    // And also we need to check if it ends with .zip, instead of any better way
                    version = FlameMod::loadIndexedPackVersion(data);
                    auto fileName = version.fileName;

                    // Have to use ensureString then use QUrl to get proper url encoding
                    dl_url = QUrl(version.downloadUrl);
                    if (!dl_url.isValid()) {
                        CustomMessageBox::selectable(
                            this, tr("Error"),
                            tr("The modpack, mod, or resource %1 is blocked for third-parties! Please download it manually.").arg(fileName),
                            QMessageBox::Critical)
                            ->show();
                        return;
                    }

                    QFileInfo dl_file(dl_url.fileName());
                });

                {  // drop stack
                    ProgressDialog dlUrlDialod(this);
                    dlUrlDialod.setSkipButton(true, tr("Abort"));
                    dlUrlDialod.execWithTask(job.get());
                }

            } else {
                dl_url = url;
            }

            if (!dl_url.isValid()) {
                continue;  // no valid url to download this resource
            }

            const QString path = dl_url.host() + '/' + dl_url.path();
            auto entry = APPLICATION->metacache()->resolveEntry("general", path);
            entry->setStale(true);
            auto dl_job = unique_qobject_ptr<NetJob>(new NetJob(tr("Modpack download"), APPLICATION->network()));
            dl_job->addNetAction(Net::ApiDownload::makeCached(dl_url, entry));
            auto archivePath = entry->getFullPath();

            bool dl_success = false;
            connect(dl_job.get(), &Task::failed, this,
                    [this](QString reason) { CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show(); });
            connect(dl_job.get(), &Task::succeeded, this, [&dl_success] { dl_success = true; });

            {  // drop stack
                ProgressDialog dlUrlDialod(this);
                dlUrlDialod.setSkipButton(true, tr("Abort"));
                dlUrlDialod.execWithTask(dl_job.get());
            }

            if (!dl_success) {
                continue;  // no local file to identify
            }
            local_url = QUrl::fromLocalFile(archivePath);

        } else {
            local_url = url;
        }

        auto localFileName = QDir::toNativeSeparators(local_url.toLocalFile());
        QFileInfo localFileInfo(localFileName);

        auto type = ResourceUtils::identify(localFileInfo);

        if (ResourceUtils::ValidResourceTypes.count(type) == 0) {  // probably instance/modpack
            addInstance(localFileName, extra_info);
            continue;
        }

        ImportResourceDialog dlg(localFileName, type, this);

        if (dlg.exec() != QDialog::Accepted)
            continue;

        qDebug() << "Adding resource" << localFileName << "to" << dlg.selectedInstanceKey;

        auto inst = APPLICATION->instances()->getInstanceById(dlg.selectedInstanceKey);
        auto minecraftInst = std::dynamic_pointer_cast<MinecraftInstance>(inst);

        switch (type) {
            case PackedResourceType::ResourcePack:
                minecraftInst->resourcePackList()->installResource(localFileName);
                break;
            case PackedResourceType::TexturePack:
                minecraftInst->texturePackList()->installResource(localFileName);
                break;
            case PackedResourceType::DataPack:
                qWarning() << "Importing of Data Packs not supported at this time. Ignoring" << localFileName;
                break;
            case PackedResourceType::Mod:
                minecraftInst->loaderModList()->installMod(localFileName, version);
                break;
            case PackedResourceType::ShaderPack:
                minecraftInst->shaderPackList()->installResource(localFileName);
                break;
            case PackedResourceType::WorldSave:
                minecraftInst->worldList()->installWorld(localFileInfo);
                break;
            case PackedResourceType::UNKNOWN:
            default:
                qDebug() << "Can't Identify" << localFileName << "Ignoring it.";
                break;
        }
    }
}

void MainWindow::on_actionREDDIT_triggered()
{
    DesktopServices::openUrl(QUrl(BuildConfig.SUBREDDIT_URL));
}

void MainWindow::on_actionDISCORD_triggered()
{
    DesktopServices::openUrl(QUrl(BuildConfig.DISCORD_URL));
}

void MainWindow::on_actionMATRIX_triggered()
{
    DesktopServices::openUrl(QUrl(BuildConfig.MATRIX_URL));
}

void MainWindow::on_actionChangeInstIcon_triggered()
{
    if (!m_selectedInstance)
        return;

    IconPickerDialog dlg(this);
    dlg.execWithSelection(m_selectedInstance->iconKey());
    if (dlg.result() == QDialog::Accepted) {
        m_selectedInstance->setIconKey(dlg.selectedIconKey);
        auto icon = APPLICATION->icons()->getIcon(dlg.selectedIconKey);
        ui->actionChangeInstIcon->setIcon(icon);
        changeIconButton->setIcon(icon);
    }
}

void MainWindow::iconUpdated(QString icon)
{
    if (icon == m_currentInstIcon) {
        auto new_icon = APPLICATION->icons()->getIcon(m_currentInstIcon);
        ui->actionChangeInstIcon->setIcon(new_icon);
        changeIconButton->setIcon(new_icon);
    }
}

void MainWindow::updateInstanceToolIcon(QString new_icon)
{
    m_currentInstIcon = new_icon;
    auto icon = APPLICATION->icons()->getIcon(m_currentInstIcon);
    ui->actionChangeInstIcon->setIcon(icon);
    changeIconButton->setIcon(icon);
}

void MainWindow::setSelectedInstanceById(const QString& id)
{
    // TODO
}

void MainWindow::on_actionChangeInstGroup_triggered()
{
    if (!m_selectedInstance)
        return;

    InstanceId instId = m_selectedInstance->id();
    QString src(APPLICATION->instances()->getInstanceGroup(instId));

    auto groups = APPLICATION->instances()->getGroups();
    groups.prepend("");
    int index = groups.indexOf(src);
    bool ok = false;
    QString dst = QInputDialog::getItem(this, tr("Category name"), tr("Enter a new category name."), groups, index, true, &ok);
    dst = dst.simplified();

    if (ok) {
        APPLICATION->instances()->setInstanceGroup(instId, dst);
    }
}

void MainWindow::deleteGroup(QString group)
{
    Q_ASSERT(!group.isEmpty());

    const int reply =
        QMessageBox::question(this, tr("Delete category"), tr("Are you sure you want to delete the category '%1'?").arg(group),
                              QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes)
        APPLICATION->instances()->deleteGroup(group);
}

void MainWindow::renameGroup(QString group)
{
    Q_ASSERT(!group.isEmpty());

    QString name = QInputDialog::getText(this, tr("Rename group"), tr("Enter a new group name."), QLineEdit::Normal, group);
    name = name.simplified();
    if (name.isNull() || name == group)
        return;

    const bool empty = name.isEmpty();
    const bool duplicate = APPLICATION->instances()->getGroups().contains(name, Qt::CaseInsensitive) && group.toLower() != name.toLower();

    if (empty || duplicate) {
        QMessageBox::warning(this, tr("Cannot rename group"), empty ? tr("Cannot set empty name.") : tr("Group already exists. :/"));
        return;
    }

    APPLICATION->instances()->renameGroup(group, name);
}

void MainWindow::undoTrashInstance()
{
    APPLICATION->instances()->undoTrashInstance();
    ui->actionUndoTrashInstance->setEnabled(APPLICATION->instances()->trashedSomething());
}

void MainWindow::on_actionViewLauncherRootFolder_triggered()
{
    DesktopServices::openDirectory(".");
}

void MainWindow::on_actionViewInstanceFolder_triggered()
{
    QString str = APPLICATION->settings()->get("InstanceDir").toString();
    DesktopServices::openDirectory(str);
}

void MainWindow::on_actionViewCentralModsFolder_triggered()
{
    DesktopServices::openDirectory(APPLICATION->settings()->get("CentralModsDir").toString(), true);
}

void MainWindow::on_actionViewIconThemeFolder_triggered()
{
    DesktopServices::openDirectory(APPLICATION->themeManager()->getIconThemesFolder().path());
}

void MainWindow::on_actionViewWidgetThemeFolder_triggered()
{
    DesktopServices::openDirectory(APPLICATION->themeManager()->getApplicationThemesFolder().path());
}

void MainWindow::on_actionViewCatPackFolder_triggered()
{
    DesktopServices::openDirectory(APPLICATION->themeManager()->getCatPacksFolder().path());
}

void MainWindow::refreshInstances()
{
    APPLICATION->instances()->loadList();
}

void MainWindow::checkForUpdates()
{
    if (APPLICATION->updaterEnabled()) {
        APPLICATION->triggerUpdateCheck();
    } else {
        qWarning() << "Updater not set up. Cannot check for updates.";
    }
}

void MainWindow::on_actionSettings_triggered()
{
    APPLICATION->ShowGlobalSettings(this, "global-settings");
}

void MainWindow::globalSettingsClosed()
{
    // FIXME: quick HACK to make this work. improve, optimize.
    APPLICATION->instances()->loadList();
    // proxymodel->invalidate();  // TODO!
    // proxymodel->sort(0);
    updateMainToolBar();
    updateLaunchButton();
    updateThemeMenu();
    // This needs to be done to prevent UI elements disappearing in the event the config is changed
    // but Prism Launcher exits abnormally, causing the window state to never be saved:
    APPLICATION->settings()->set("MainWindowState", saveState().toBase64());
    update();
}

void MainWindow::on_actionEditInstance_triggered()
{
    if (!m_selectedInstance)
        return;

    if (m_selectedInstance->canEdit()) {
        APPLICATION->showInstanceWindow(m_selectedInstance);
    } else {
        CustomMessageBox::selectable(this, tr("Instance not editable"),
                                     tr("This instance is not editable. It may be broken, invalid, or too old. Check logs for details."),
                                     QMessageBox::Critical)
            ->show();
    }
}

void MainWindow::on_actionManageAccounts_triggered()
{
    APPLICATION->ShowGlobalSettings(this, "accounts");
}

void MainWindow::on_actionReportBug_triggered()
{
    DesktopServices::openUrl(QUrl(BuildConfig.BUG_TRACKER_URL));
}

void MainWindow::on_actionClearMetadata_triggered()
{
    APPLICATION->metacache()->evictAll();
    APPLICATION->metacache()->SaveNow();
}

#ifdef Q_OS_MAC
void MainWindow::on_actionAddToPATH_triggered()
{
    auto binaryPath = APPLICATION->applicationFilePath();
    auto targetPath = QString("/usr/local/bin/%1").arg(BuildConfig.LAUNCHER_APP_BINARY_NAME);
    qDebug() << "Symlinking" << binaryPath << "to" << targetPath;

    QStringList args;
    args << "-e";
    args << QString("do shell script \"mkdir -p /usr/local/bin && ln -sf '%1' '%2'\" with administrator privileges")
                .arg(binaryPath, targetPath);
    auto outcome = QProcess::execute("/usr/bin/osascript", args);
    if (!outcome) {
        QMessageBox::information(this, tr("Successfully added %1 to PATH").arg(BuildConfig.LAUNCHER_DISPLAYNAME),
                                 tr("%1 was successfully added to your PATH. You can now start it by running `%2`.")
                                     .arg(BuildConfig.LAUNCHER_DISPLAYNAME, BuildConfig.LAUNCHER_APP_BINARY_NAME));
    } else {
        QMessageBox::critical(this, tr("Failed to add %1 to PATH").arg(BuildConfig.LAUNCHER_DISPLAYNAME),
                              tr("An error occurred while trying to add %1 to PATH").arg(BuildConfig.LAUNCHER_DISPLAYNAME));
    }
}
#endif

void MainWindow::on_actionOpenWiki_triggered()
{
    DesktopServices::openUrl(QUrl(BuildConfig.HELP_URL.arg("")));
}

void MainWindow::on_actionMoreNews_triggered()
{
    auto entries = m_newsChecker->getNewsEntries();
    NewsDialog news_dialog(entries, this);
    news_dialog.exec();
}

void MainWindow::newsButtonClicked()
{
    auto entries = m_newsChecker->getNewsEntries();
    NewsDialog news_dialog(entries, this);
    news_dialog.toggleArticleList();
    news_dialog.exec();
}

void MainWindow::onCatChanged(int)
{
    view->setCatDisplayed(APPLICATION->settings()->get("TheCat").toBool());
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::on_actionDeleteInstance_triggered()
{
    if (!m_selectedInstance) {
        return;
    }

    auto id = m_selectedInstance->id();

    auto response = CustomMessageBox::selectable(this, tr("Confirm Deletion"),
                                                 tr("You are about to delete \"%1\".\n"
                                                    "This may be permanent and will completely delete the instance.\n\n"
                                                    "Are you sure?")
                                                     .arg(m_selectedInstance->name()),
                                                 QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                        ->exec();

    if (response != QMessageBox::Yes)
        return;

    auto linkedInstances = APPLICATION->instances()->getLinkedInstancesById(id);
    if (!linkedInstances.empty()) {
        response = CustomMessageBox::selectable(this, tr("There are linked instances"),
                                                tr("The following instance(s) might reference files in this instance:\n\n"
                                                   "%1\n\n"
                                                   "Deleting it could break the other instance(s), \n\n"
                                                   "Do you wish to proceed?",
                                                   nullptr, linkedInstances.count())
                                                    .arg(linkedInstances.join("\n")),
                                                QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                       ->exec();
        if (response != QMessageBox::Yes)
            return;
    }

    if (APPLICATION->instances()->trashInstance(id)) {
        ui->actionUndoTrashInstance->setEnabled(APPLICATION->instances()->trashedSomething());
    } else {
        APPLICATION->instances()->deleteInstance(id);
    }
    APPLICATION->settings()->set("SelectedInstance", QString());
    selectionBad();
}

void MainWindow::on_actionExportInstanceZip_triggered()
{
    if (m_selectedInstance) {
        ExportInstanceDialog dlg(m_selectedInstance, this);
        dlg.exec();
    }
}

void MainWindow::on_actionExportInstanceMrPack_triggered()
{
    if (m_selectedInstance) {
        ExportPackDialog dlg(m_selectedInstance, this);
        dlg.exec();
    }
}

void MainWindow::on_actionExportInstanceToModList_triggered()
{
    if (m_selectedInstance) {
        ExportToModListDialog dlg(m_selectedInstance, this);
        dlg.exec();
    }
}

void MainWindow::on_actionExportInstanceFlamePack_triggered()
{
    if (m_selectedInstance) {
        auto instance = dynamic_cast<MinecraftInstance*>(m_selectedInstance.get());
        if (instance) {
            if (auto cmp = instance->getPackProfile()->getComponent("net.minecraft");
                cmp && cmp->getVersionFile() && cmp->getVersionFile()->type == "snapshot") {
                QMessageBox msgBox(this);
                msgBox.setText("Snapshots are currently not supported by CurseForge modpacks.");
                msgBox.exec();
                return;
            }
            ExportPackDialog dlg(m_selectedInstance, this, ModPlatform::ResourceProvider::FLAME);
            dlg.exec();
        }
    }
}

void MainWindow::on_actionRenameInstance_triggered()
{
    // NOTE: editSelected already checky if an instance is selected
    view->editSelected();
}

void MainWindow::on_actionViewSelectedInstFolder_triggered()
{
    if (m_selectedInstance) {
        QString str = m_selectedInstance->instanceRoot();
        DesktopServices::openDirectory(QDir(str).absolutePath());
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // Save the window state and geometry.
    APPLICATION->settings()->set("MainWindowState", saveState().toBase64());
    APPLICATION->settings()->set("MainWindowGeometry", saveGeometry().toBase64());
    view->storeState();
    instanceToolbarSetting->set(ui->instanceToolBar->getVisibilityState());
    event->accept();
    emit isClosing();
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::instanceActivated(InstancePtr inst)
{
    if (!inst)
        return;

    activateInstance(inst);
}

void MainWindow::on_actionLaunchInstance_triggered()
{
    if (m_selectedInstance && !m_selectedInstance->isRunning()) {
        APPLICATION->launch(m_selectedInstance);
    }
}

void MainWindow::activateInstance(InstancePtr instance)
{
    APPLICATION->launch(instance);
}

void MainWindow::on_actionKillInstance_triggered()
{
    if (m_selectedInstance && m_selectedInstance->isRunning()) {
        APPLICATION->kill(m_selectedInstance);
    }
}

void MainWindow::on_actionCreateInstanceShortcut_triggered()
{
    if (!m_selectedInstance)
        return;
    auto desktopPath = FS::getDesktopDir();
    if (desktopPath.isEmpty()) {
        // TODO come up with an alternative solution (open "save file" dialog)
        QMessageBox::critical(this, tr("Create instance shortcut"), tr("Couldn't find desktop?!"));
        return;
    }

    QString desktopFilePath;
    QString appPath = QApplication::applicationFilePath();
    QString iconPath;
    QStringList args;
#if defined(Q_OS_MACOS)
    appPath = QApplication::applicationFilePath();
    if (appPath.startsWith("/private/var/")) {
        QMessageBox::critical(this, tr("Create instance shortcut"),
                              tr("The launcher is in the folder it was extracted from, therefore it cannot create shortcuts."));
        return;
    }

    auto pIcon = APPLICATION->icons()->icon(m_selectedInstance->iconKey());
    if (pIcon == nullptr) {
        pIcon = APPLICATION->icons()->icon("grass");
    }

    iconPath = FS::PathCombine(m_selectedInstance->instanceRoot(), "Icon.icns");

    QFile iconFile(iconPath);
    if (!iconFile.open(QFile::WriteOnly)) {
        QMessageBox::critical(this, tr("Create instance Application"), tr("Failed to create icon for Application."));
        return;
    }

    QIcon icon = pIcon->icon();

    bool success = icon.pixmap(1024, 1024).save(iconPath, "ICNS");
    iconFile.close();

    if (!success) {
        iconFile.remove();
        QMessageBox::critical(this, tr("Create instance Application"), tr("Failed to create icon for Application."));
        return;
    }
#elif defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
    if (appPath.startsWith("/tmp/.mount_")) {
        // AppImage!
        appPath = QProcessEnvironment::systemEnvironment().value(QStringLiteral("APPIMAGE"));
        if (appPath.isEmpty()) {
            QMessageBox::critical(this, tr("Create instance shortcut"),
                                  tr("Launcher is running as misconfigured AppImage? ($APPIMAGE environment variable is missing)"));
        } else if (appPath.endsWith("/")) {
            appPath.chop(1);
        }
    }

    auto icon = APPLICATION->icons()->icon(m_selectedInstance->iconKey());
    if (icon == nullptr) {
        icon = APPLICATION->icons()->icon("grass");
    }

    iconPath = FS::PathCombine(m_selectedInstance->instanceRoot(), "icon.png");

    QFile iconFile(iconPath);
    if (!iconFile.open(QFile::WriteOnly)) {
        QMessageBox::critical(this, tr("Create instance shortcut"), tr("Failed to create icon for shortcut."));
        return;
    }
    bool success = icon->icon().pixmap(64, 64).save(&iconFile, "PNG");
    iconFile.close();

    if (!success) {
        iconFile.remove();
        QMessageBox::critical(this, tr("Create instance shortcut"), tr("Failed to create icon for shortcut."));
        return;
    }

    if (DesktopServices::isFlatpak()) {
        desktopFilePath = FS::PathCombine(desktopPath, FS::RemoveInvalidFilenameChars(m_selectedInstance->name()) + ".desktop");
        QFileDialog fileDialog;
        // workaround to make sure the portal file dialog opens in the desktop directory
        fileDialog.setDirectoryUrl(desktopPath);
        desktopFilePath = fileDialog.getSaveFileName(this, tr("Create Shortcut"), desktopFilePath, tr("Desktop Entries (*.desktop)"));
        if (desktopFilePath.isEmpty())
            return;  // file dialog canceled by user
        appPath = "flatpak";
        QString flatpakAppId = BuildConfig.LAUNCHER_DESKTOPFILENAME;
        flatpakAppId.remove(".desktop");
        args.append({ "run", flatpakAppId });
    }

    if (FS::createShortcut(FS::PathCombine(desktopPath, m_selectedInstance->name()), appPath, { "--launch", m_selectedInstance->id() },
                           m_selectedInstance->name(), iconPath)) {
        QMessageBox::information(this, tr("Create instance shortcut"), tr("Created a shortcut to this instance on your desktop!"));
    } else {
        iconFile.remove();
        QMessageBox::critical(this, tr("Create instance shortcut"), tr("Failed to create instance shortcut!"));
    }
#elif defined(Q_OS_WIN)
    auto icon = APPLICATION->icons()->icon(m_selectedInstance->iconKey());
    if (icon == nullptr) {
        icon = APPLICATION->icons()->icon("grass");
    }

    QString iconPath = FS::PathCombine(m_selectedInstance->instanceRoot(), "icon.ico");

    // part of fix for weird bug involving the window icon being replaced
    // dunno why it happens, but this 2-line fix seems to be enough, so w/e
    auto appIcon = APPLICATION->getThemedIcon("logo");

    // part of fix for weird bug involving the window icon being replaced
    // dunno why it happens, but this 2-line fix seems to be enough, so w/e
    auto appIcon = APPLICATION->getThemedIcon("logo");

    QFile iconFile(iconPath);
    if (!iconFile.open(QFile::WriteOnly)) {
        QMessageBox::critical(this, tr("Create instance shortcut"), tr("Failed to create icon for shortcut."));
        return;
    }
    bool success = icon->icon().pixmap(64, 64).save(&iconFile, "ICO");
    iconFile.close();

    // restore original window icon
    QGuiApplication::setWindowIcon(appIcon);

    if (!success) {
        iconFile.remove();
        QMessageBox::critical(this, tr("Create instance shortcut"), tr("Failed to create icon for shortcut."));
        return;
    }

    if (!success) {
        iconFile.remove();
        QMessageBox::critical(this, tr("Create instance shortcut"), tr("Failed to create icon for shortcut."));
        return;
    }

    if (FS::createShortcut(FS::PathCombine(desktopPath, m_selectedInstance->name()), QApplication::applicationFilePath(),
                           { "--launch", m_selectedInstance->id() }, m_selectedInstance->name(), iconPath)) {
        QMessageBox::information(this, tr("Create instance shortcut"), tr("Created a shortcut to this instance on your desktop!"));
    } else {
        iconFile.remove();
        QMessageBox::critical(this, tr("Create instance shortcut"), tr("Failed to create instance shortcut!"));
    }
#else
    QMessageBox::critical(this, tr("Create instance shortcut"), tr("Not supported on your platform!"));
    return;
#endif
    args.append({ "--launch", m_selectedInstance->id() });
    if (FS::createShortcut(desktopFilePath, appPath, args, m_selectedInstance->name(), iconPath)) {
#if not defined(Q_OS_MACOS)
        QMessageBox::information(this, tr("Create instance shortcut"), tr("Created a shortcut to this instance on your desktop!"));
#else
        QMessageBox::information(this, tr("Create instance shortcut"), tr("Created a shortcut to this instance!"));
#endif
    } else {
#if not defined(Q_OS_MACOS)
        iconFile.remove();
#endif
        QMessageBox::critical(this, tr("Create instance shortcut"), tr("Failed to create instance shortcut!"));
    }
}

void MainWindow::taskEnd()
{
    QObject* sender = QObject::sender();
    if (sender == m_versionLoadTask)
        m_versionLoadTask = NULL;

    sender->deleteLater();
}

void MainWindow::startTask(Task* task)
{
    connect(task, SIGNAL(succeeded()), SLOT(taskEnd()));
    connect(task, SIGNAL(failed(QString)), SLOT(taskEnd()));
    task->start();
}

void MainWindow::instanceChanged(InstancePtr current, [[maybe_unused]] InstancePtr previous)
{
    if (!current) {
        APPLICATION->settings()->set("SelectedInstance", QString());
        selectionBad();
        return;
    }
    if (m_selectedInstance) {
        disconnect(m_selectedInstance.get(), &BaseInstance::runningStatusChanged, this, &MainWindow::refreshCurrentInstance);
        disconnect(m_selectedInstance.get(), &BaseInstance::profilerChanged, this, &MainWindow::refreshCurrentInstance);
    }
    m_selectedInstance = current;
    if (m_selectedInstance) {
        ui->instanceToolBar->setEnabled(true);
        setInstanceActionsEnabled(true);
        ui->actionLaunchInstance->setEnabled(m_selectedInstance->canLaunch());

        ui->actionKillInstance->setEnabled(m_selectedInstance->isRunning());
        ui->actionExportInstance->setEnabled(m_selectedInstance->canExport());
        renameButton->setText(m_selectedInstance->name());
        updateInstanceToolIcon(m_selectedInstance->iconKey());

        updateLaunchButton();

        APPLICATION->settings()->set("SelectedInstance", m_selectedInstance->id());

        connect(m_selectedInstance.get(), &BaseInstance::runningStatusChanged, this, &MainWindow::refreshCurrentInstance);
        connect(m_selectedInstance.get(), &BaseInstance::profilerChanged, this, &MainWindow::refreshCurrentInstance);
    } else {
        APPLICATION->settings()->set("SelectedInstance", QString());
        selectionBad();
        return;
    }
}

void MainWindow::instanceSelectRequest(QString id)
{
    setSelectedInstanceById(id);
}

void MainWindow::selectionBad()
{
    // start by reseting everything...
    m_selectedInstance = nullptr;

    ui->instanceToolBar->setEnabled(false);
    setInstanceActionsEnabled(false);
    updateLaunchButton();
    renameButton->setText(tr("Rename Instance"));
    updateInstanceToolIcon("grass");

    // ...and then see if we can enable the previously selected instance
    setSelectedInstanceById(APPLICATION->settings()->get("SelectedInstance").toString());
}

void MainWindow::checkInstancePathForProblems()
{
    QString instanceFolder = APPLICATION->settings()->get("InstanceDir").toString();
    if (FS::checkProblemticPathJava(QDir(instanceFolder))) {
        QMessageBox warning(this);
        warning.setText(tr("Your instance folder contains \'!\' and this is known to cause Java problems!"));
        warning.setInformativeText(tr("You have now two options: <br/>"
                                      " - change the instance folder in the settings <br/>"
                                      " - move this installation of %1 to a different folder")
                                       .arg(BuildConfig.LAUNCHER_DISPLAYNAME));
        warning.setDefaultButton(QMessageBox::Ok);
        warning.exec();
    }
    auto tempFolderText =
        tr("This is a problem: <br/>"
           " - The launcher will likely be deleted without warning by the operating system <br/>"
           " - close the launcher now and extract it to a real location, not a temporary folder");
    QString pathfoldername = QDir(instanceFolder).absolutePath();
    if (pathfoldername.contains("Rar$", Qt::CaseInsensitive)) {
        QMessageBox warning(this);
        warning.setText(tr("Your instance folder contains \'Rar$\' - that means you haven't extracted the launcher archive!"));
        warning.setInformativeText(tempFolderText);
        warning.setDefaultButton(QMessageBox::Ok);
        warning.exec();
    } else if (pathfoldername.startsWith(QDir::tempPath()) || pathfoldername.contains("/TempState/")) {
        QMessageBox warning(this);
        warning.setText(tr("Your instance folder is in a temporary folder: \'%1\'!").arg(QDir::tempPath()));
        warning.setInformativeText(tempFolderText);
        warning.setDefaultButton(QMessageBox::Ok);
        warning.exec();
    }
}

// "Instance actions" are actions that require an instance to be selected (i.e. "new instance" is not here)
// Actions that also require other conditions (e.g. a running instance) won't be changed.
void MainWindow::setInstanceActionsEnabled(bool enabled)
{
    ui->actionEditInstance->setEnabled(enabled);
    ui->actionChangeInstGroup->setEnabled(enabled);
    ui->actionViewSelectedInstFolder->setEnabled(enabled);
    ui->actionExportInstance->setEnabled(enabled);
    ui->actionDeleteInstance->setEnabled(enabled);
    ui->actionCopyInstance->setEnabled(enabled);
    ui->actionCreateInstanceShortcut->setEnabled(enabled);
}

void MainWindow::refreshCurrentInstance()
{
    auto current = view->currentInstance();
    instanceChanged(current, current);
}
