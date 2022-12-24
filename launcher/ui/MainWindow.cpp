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

#include "Application.h"
#include "BuildConfig.h"
#include "FileSystem.h"

#include "MainWindow.h"

#include <QVariant>
#include <QUrl>
#include <QDir>
#include <QFileInfo>

#include <QKeyEvent>
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMainWindow>
#include <QStatusBar>
#include <QToolBar>
#include <QWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QToolButton>
#include <QWidgetAction>
#include <QProgressDialog>
#include <QShortcut>

#include <BaseInstance.h>
#include <InstanceList.h>
#include <minecraft/MinecraftInstance.h>
#include <MMCZip.h>
#include <icons/IconList.h>
#include <java/JavaUtils.h>
#include <java/JavaInstallList.h>
#include <launch/LaunchTask.h>
#include <minecraft/auth/AccountList.h>
#include <SkinUtils.h>
#include <BuildConfig.h>
#include <net/NetJob.h>
#include <net/Download.h>
#include <news/NewsChecker.h>
#include <tools/BaseProfiler.h>
#include <updater/DownloadTask.h>
#include <updater/UpdateChecker.h>
#include <DesktopServices.h>
#include "InstanceWindow.h"
#include "InstancePageProvider.h"
#include "JavaCommon.h"
#include "LaunchController.h"

#include "ui/instanceview/InstanceProxyModel.h"
#include "ui/instanceview/InstanceView.h"
#include "ui/instanceview/InstanceDelegate.h"
#include "ui/widgets/LabeledToolButton.h"
#include "ui/dialogs/NewInstanceDialog.h"
#include "ui/dialogs/NewsDialog.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/dialogs/AboutDialog.h"
#include "ui/dialogs/VersionSelectDialog.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/IconPickerDialog.h"
#include "ui/dialogs/CopyInstanceDialog.h"
#include "ui/dialogs/UpdateDialog.h"
#include "ui/dialogs/EditAccountDialog.h"
#include "ui/dialogs/ExportInstanceDialog.h"
#include "ui/dialogs/ImportResourcePackDialog.h"
#include "ui/themes/ITheme.h"

#include <minecraft/mod/ResourcePackFolderModel.h>
#include <minecraft/mod/tasks/LocalResourcePackParseTask.h>
#include <minecraft/mod/TexturePackFolderModel.h>
#include <minecraft/mod/tasks/LocalTexturePackParseTask.h>

#include "UpdateController.h"
#include "KonamiCode.h"

#include "InstanceImportTask.h"
#include "InstanceCopyTask.h"

#include "MMCTime.h"

namespace {
QString profileInUseFilter(const QString & profile, bool used)
{
    if(used)
    {
        return QObject::tr("%1 (in use)").arg(profile);
    }
    else
    {
        return profile;
    }
}
}

// WHY: to hold the pre-translation strings together with the T pointer, so it can be retranslated without a lot of ugly code
template <typename T>
class Translated
{
public:
    Translated(){}
    Translated(QWidget *parent)
    {
        m_contained = new T(parent);
    }
    void setTooltipId(const char * tooltip)
    {
        m_tooltip = tooltip;
    }
    void setTextId(const char * text)
    {
        m_text = text;
    }
    operator T*()
    {
        return m_contained;
    }
    T * operator->()
    {
        return m_contained;
    }
    void retranslate()
    {
        if(m_text)
        {
            QString result;
            result = QApplication::translate("MainWindow", m_text);
            if(result.contains("%1")) {
                result = result.arg(BuildConfig.LAUNCHER_DISPLAYNAME);
            }
            m_contained->setText(result);
        }
        if(m_tooltip)
        {
            QString result;
            result = QApplication::translate("MainWindow", m_tooltip);
            if(result.contains("%1")) {
                result = result.arg(BuildConfig.LAUNCHER_DISPLAYNAME);
            }
            m_contained->setToolTip(result);
        }
    }
private:
    T * m_contained = nullptr;
    const char * m_text = nullptr;
    const char * m_tooltip = nullptr;
};
using TranslatedAction = Translated<QAction>;
using TranslatedToolButton = Translated<QToolButton>;

class TranslatedToolbar
{
public:
    TranslatedToolbar(){}
    TranslatedToolbar(QWidget *parent)
    {
        m_contained = new QToolBar(parent);
    }
    void setWindowTitleId(const char * title)
    {
        m_title = title;
    }
    operator QToolBar*()
    {
        return m_contained;
    }
    QToolBar * operator->()
    {
        return m_contained;
    }
    void retranslate()
    {
        if(m_title)
        {
            m_contained->setWindowTitle(QApplication::translate("MainWindow", m_title));
        }
    }
private:
    QToolBar * m_contained = nullptr;
    const char * m_title = nullptr;
};

class MainWindow::Ui
{
public:
    TranslatedAction actionAddInstance;
    //TranslatedAction actionRefresh;
    TranslatedAction actionCheckUpdate;
    TranslatedAction actionSettings;
    TranslatedAction actionMoreNews;
    TranslatedAction actionManageAccounts;
    TranslatedAction actionLaunchInstance;
    TranslatedAction actionKillInstance;
    TranslatedAction actionRenameInstance;
    TranslatedAction actionChangeInstGroup;
    TranslatedAction actionChangeInstIcon;
    TranslatedAction actionEditInstance;
    TranslatedAction actionViewSelectedInstFolder;
    TranslatedAction actionDeleteInstance;
    TranslatedAction actionCAT;
    TranslatedAction actionCopyInstance;
    TranslatedAction actionLaunchInstanceOffline;
    TranslatedAction actionLaunchInstanceDemo;
    TranslatedAction actionExportInstance;
    TranslatedAction actionCreateInstanceShortcut;
    QVector<TranslatedAction *> all_actions;

    LabeledToolButton *renameButton = nullptr;
    LabeledToolButton *changeIconButton = nullptr;

    QMenu * foldersMenu = nullptr;
    TranslatedToolButton foldersMenuButton;
    TranslatedAction actionViewInstanceFolder;
    TranslatedAction actionViewCentralModsFolder;

    QMenu * editMenu = nullptr;
    TranslatedAction actionUndoTrashInstance;

    QMenu * helpMenu = nullptr;
    TranslatedToolButton helpMenuButton;
    TranslatedAction actionClearMetadata;
    #ifdef Q_OS_MAC
    TranslatedAction actionAddToPATH;
    #endif
    TranslatedAction actionReportBug;
    TranslatedAction actionDISCORD;
    TranslatedAction actionMATRIX;
    TranslatedAction actionREDDIT;
    TranslatedAction actionAbout;

    TranslatedAction actionNoAccountsAdded;
    TranslatedAction actionNoDefaultAccount;

    TranslatedAction actionLockToolbars;

    TranslatedAction actionChangeTheme;

    QVector<TranslatedToolButton *> all_toolbuttons;

    QWidget *centralWidget = nullptr;
    QHBoxLayout *horizontalLayout = nullptr;
    QStatusBar *statusBar = nullptr;

    QMenuBar *menuBar = nullptr;
    QMenu *fileMenu;
    QMenu *viewMenu;
    QMenu *profileMenu;

    TranslatedAction actionCloseWindow;

    TranslatedAction actionOpenWiki;
    TranslatedAction actionNewsMenuBar;

    TranslatedToolbar mainToolBar;
    TranslatedToolbar instanceToolBar;
    TranslatedToolbar newsToolBar;
    QVector<TranslatedToolbar *> all_toolbars;

    void createMainToolbarActions(MainWindow *MainWindow)
    {
        actionAddInstance = TranslatedAction(MainWindow);
        actionAddInstance->setObjectName(QStringLiteral("actionAddInstance"));
        actionAddInstance->setIcon(APPLICATION->getThemedIcon("new"));
        actionAddInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Add Instanc&e..."));
        actionAddInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Add a new instance."));
        actionAddInstance->setShortcut(QKeySequence::New);
        all_actions.append(&actionAddInstance);

        actionViewInstanceFolder = TranslatedAction(MainWindow);
        actionViewInstanceFolder->setObjectName(QStringLiteral("actionViewInstanceFolder"));
        actionViewInstanceFolder->setIcon(APPLICATION->getThemedIcon("viewfolder"));
        actionViewInstanceFolder.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&View Instance Folder"));
        actionViewInstanceFolder.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the instance folder in a file browser."));
        all_actions.append(&actionViewInstanceFolder);

        actionViewCentralModsFolder = TranslatedAction(MainWindow);
        actionViewCentralModsFolder->setObjectName(QStringLiteral("actionViewCentralModsFolder"));
        actionViewCentralModsFolder->setIcon(APPLICATION->getThemedIcon("centralmods"));
        actionViewCentralModsFolder.setTextId(QT_TRANSLATE_NOOP("MainWindow", "View &Central Mods Folder"));
        actionViewCentralModsFolder.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the central mods folder in a file browser."));
        all_actions.append(&actionViewCentralModsFolder);

        foldersMenu = new QMenu(MainWindow);
        foldersMenu->setTitle(tr("F&olders"));
        foldersMenu->setToolTipsVisible(true);

        foldersMenu->addAction(actionViewInstanceFolder);
        foldersMenu->addAction(actionViewCentralModsFolder);

        foldersMenuButton = TranslatedToolButton(MainWindow);
        foldersMenuButton.setTextId(QT_TRANSLATE_NOOP("MainWindow", "F&olders"));
        foldersMenuButton.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open one of the folders shared between instances."));
        foldersMenuButton->setMenu(foldersMenu);
        foldersMenuButton->setPopupMode(QToolButton::InstantPopup);
        foldersMenuButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        foldersMenuButton->setIcon(APPLICATION->getThemedIcon("viewfolder"));
        foldersMenuButton->setFocusPolicy(Qt::NoFocus);
        all_toolbuttons.append(&foldersMenuButton);

        actionSettings = TranslatedAction(MainWindow);
        actionSettings->setObjectName(QStringLiteral("actionSettings"));
        actionSettings->setIcon(APPLICATION->getThemedIcon("settings"));
        actionSettings->setMenuRole(QAction::PreferencesRole);
        actionSettings.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Setti&ngs..."));
        actionSettings.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Change settings."));
        actionSettings->setShortcut(QKeySequence::Preferences);
        all_actions.append(&actionSettings);

        actionUndoTrashInstance = TranslatedAction(MainWindow);
        actionUndoTrashInstance->setObjectName(QStringLiteral("actionUndoTrashInstance"));
        actionUndoTrashInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Undo Last Instance Deletion"));
        actionUndoTrashInstance->setEnabled(APPLICATION->instances()->trashedSomething());
        actionUndoTrashInstance->setShortcut(QKeySequence::Undo);
        all_actions.append(&actionUndoTrashInstance);

        actionClearMetadata = TranslatedAction(MainWindow);
        actionClearMetadata->setObjectName(QStringLiteral("actionClearMetadata"));
        actionClearMetadata->setIcon(APPLICATION->getThemedIcon("refresh"));
        actionClearMetadata.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Clear Metadata Cache"));
        actionClearMetadata.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Clear cached metadata"));
        all_actions.append(&actionClearMetadata);

        #ifdef Q_OS_MAC
        actionAddToPATH = TranslatedAction(MainWindow);
        actionAddToPATH->setObjectName(QStringLiteral("actionAddToPATH"));
        actionAddToPATH.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Install to &PATH"));
        actionAddToPATH.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Install a prismlauncher symlink to /usr/local/bin"));
        all_actions.append(&actionAddToPATH);
        #endif

        if (!BuildConfig.BUG_TRACKER_URL.isEmpty()) {
            actionReportBug = TranslatedAction(MainWindow);
            actionReportBug->setObjectName(QStringLiteral("actionReportBug"));
            actionReportBug->setIcon(APPLICATION->getThemedIcon("bug"));
            actionReportBug.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Report a &Bug..."));
            actionReportBug.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the bug tracker to report a bug with %1."));
            all_actions.append(&actionReportBug);
        }

        if(!BuildConfig.MATRIX_URL.isEmpty()) {
            actionMATRIX = TranslatedAction(MainWindow);
            actionMATRIX->setObjectName(QStringLiteral("actionMATRIX"));
            actionMATRIX->setIcon(APPLICATION->getThemedIcon("matrix"));
            actionMATRIX.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Matrix Space"));
            actionMATRIX.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open %1 Matrix space"));
            all_actions.append(&actionMATRIX);
        }

        if (!BuildConfig.DISCORD_URL.isEmpty()) {
            actionDISCORD = TranslatedAction(MainWindow);
            actionDISCORD->setObjectName(QStringLiteral("actionDISCORD"));
            actionDISCORD->setIcon(APPLICATION->getThemedIcon("discord"));
            actionDISCORD.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Discord Guild"));
            actionDISCORD.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open %1 Discord guild."));
            all_actions.append(&actionDISCORD);
        }

        if (!BuildConfig.SUBREDDIT_URL.isEmpty()) {
            actionREDDIT = TranslatedAction(MainWindow);
            actionREDDIT->setObjectName(QStringLiteral("actionREDDIT"));
            actionREDDIT->setIcon(APPLICATION->getThemedIcon("reddit-alien"));
            actionREDDIT.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Sub&reddit"));
            actionREDDIT.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open %1 subreddit."));
            all_actions.append(&actionREDDIT);
        }

        actionAbout = TranslatedAction(MainWindow);
        actionAbout->setObjectName(QStringLiteral("actionAbout"));
        actionAbout->setIcon(APPLICATION->getThemedIcon("about"));
        actionAbout->setMenuRole(QAction::AboutRole);
        actionAbout.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&About %1"));
        actionAbout.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "View information about %1."));
        all_actions.append(&actionAbout);

        if(BuildConfig.UPDATER_ENABLED)
        {
            actionCheckUpdate = TranslatedAction(MainWindow);
            actionCheckUpdate->setObjectName(QStringLiteral("actionCheckUpdate"));
            actionCheckUpdate->setIcon(APPLICATION->getThemedIcon("checkupdate"));
            actionCheckUpdate.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Update..."));
            actionCheckUpdate.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Check for new updates for %1."));
            actionCheckUpdate->setMenuRole(QAction::ApplicationSpecificRole);
            all_actions.append(&actionCheckUpdate);
        }

        actionCAT = TranslatedAction(MainWindow);
        actionCAT->setObjectName(QStringLiteral("actionCAT"));
        actionCAT->setCheckable(true);
        actionCAT->setIcon(APPLICATION->getThemedIcon("cat"));
        actionCAT.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Meow"));
        actionCAT.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "It's a fluffy kitty :3"));
        actionCAT->setPriority(QAction::LowPriority);
        all_actions.append(&actionCAT);

        // profile menu and its actions
        actionManageAccounts = TranslatedAction(MainWindow);
        actionManageAccounts->setObjectName(QStringLiteral("actionManageAccounts"));
        actionManageAccounts.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Manage Accounts..."));
        // FIXME: no tooltip!
        actionManageAccounts->setCheckable(false);
        actionManageAccounts->setIcon(APPLICATION->getThemedIcon("accounts"));
        all_actions.append(&actionManageAccounts);

        actionLockToolbars = TranslatedAction(MainWindow);
        actionLockToolbars->setObjectName(QStringLiteral("actionLockToolbars"));
        actionLockToolbars.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Lock Toolbars"));
        actionLockToolbars->setCheckable(true);
        all_actions.append(&actionLockToolbars);

        actionChangeTheme = TranslatedAction(MainWindow);
        actionChangeTheme->setObjectName(QStringLiteral("actionChangeTheme"));
        actionChangeTheme.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Themes"));
        all_actions.append(&actionChangeTheme);
    }

    void createMainToolbar(QMainWindow *MainWindow)
    {
        mainToolBar = TranslatedToolbar(MainWindow);
        mainToolBar->setVisible(menuBar->isNativeMenuBar() || !APPLICATION->settings()->get("MenuBarInsteadOfToolBar").toBool());
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        mainToolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
        mainToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        mainToolBar->setFloatable(false);
        mainToolBar.setWindowTitleId(QT_TRANSLATE_NOOP("MainWindow", "Main Toolbar"));

        mainToolBar->addAction(actionAddInstance);

        mainToolBar->addSeparator();

        QWidgetAction* foldersButtonAction = new QWidgetAction(MainWindow);
        foldersButtonAction->setDefaultWidget(foldersMenuButton);
        mainToolBar->addAction(foldersButtonAction);

        mainToolBar->addAction(actionSettings);

        helpMenu = new QMenu(MainWindow);
        helpMenu->setToolTipsVisible(true);

        helpMenu->addAction(actionClearMetadata);

        #ifdef Q_OS_MAC
        helpMenu->addAction(actionAddToPATH);
        #endif

        if (!BuildConfig.BUG_TRACKER_URL.isEmpty()) {
            helpMenu->addAction(actionReportBug);
        }
        
        if(!BuildConfig.MATRIX_URL.isEmpty()) {
            helpMenu->addAction(actionMATRIX);
        }

        if (!BuildConfig.DISCORD_URL.isEmpty()) {
            helpMenu->addAction(actionDISCORD);
        }

        if (!BuildConfig.SUBREDDIT_URL.isEmpty()) {
            helpMenu->addAction(actionREDDIT);
        }

        helpMenu->addAction(actionAbout);

        helpMenuButton = TranslatedToolButton(MainWindow);
        helpMenuButton.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Help"));
        helpMenuButton.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Get help with %1 or Minecraft."));
        helpMenuButton->setMenu(helpMenu);
        helpMenuButton->setPopupMode(QToolButton::InstantPopup);
        helpMenuButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        helpMenuButton->setIcon(APPLICATION->getThemedIcon("help"));
        helpMenuButton->setFocusPolicy(Qt::NoFocus);
        all_toolbuttons.append(&helpMenuButton);
        QWidgetAction* helpButtonAction = new QWidgetAction(MainWindow);
        helpButtonAction->setDefaultWidget(helpMenuButton);
        mainToolBar->addAction(helpButtonAction);

        if(BuildConfig.UPDATER_ENABLED)
        {
            mainToolBar->addAction(actionCheckUpdate);
        }

        mainToolBar->addSeparator();

        mainToolBar->addAction(actionCAT);

        all_toolbars.append(&mainToolBar);
        MainWindow->addToolBar(Qt::TopToolBarArea, mainToolBar);
    }

    void createMenuBar(QMainWindow *MainWindow)
    {
        menuBar = new QMenuBar(MainWindow);
        menuBar->setVisible(APPLICATION->settings()->get("MenuBarInsteadOfToolBar").toBool());

        fileMenu = menuBar->addMenu(tr("&File"));
        // Workaround for QTBUG-94802 (https://bugreports.qt.io/browse/QTBUG-94802); also present for other menus
        fileMenu->setSeparatorsCollapsible(false);
        fileMenu->addAction(actionAddInstance);
        fileMenu->addAction(actionLaunchInstance);
        fileMenu->addAction(actionKillInstance);
        fileMenu->addAction(actionCloseWindow);
        fileMenu->addSeparator();
        fileMenu->addAction(actionEditInstance);
        fileMenu->addAction(actionChangeInstGroup);
        fileMenu->addAction(actionViewSelectedInstFolder);
        fileMenu->addAction(actionExportInstance);
        fileMenu->addAction(actionCopyInstance);
        fileMenu->addAction(actionDeleteInstance);
        fileMenu->addAction(actionCreateInstanceShortcut);
        fileMenu->addSeparator();
        fileMenu->addAction(actionSettings);

        editMenu = menuBar->addMenu(tr("&Edit"));
        editMenu->addAction(actionUndoTrashInstance);

        viewMenu = menuBar->addMenu(tr("&View"));
        viewMenu->setSeparatorsCollapsible(false);
        viewMenu->addAction(actionChangeTheme);
        viewMenu->addSeparator();
        viewMenu->addAction(actionCAT);
        viewMenu->addSeparator();

        viewMenu->addAction(actionLockToolbars);

        menuBar->addMenu(foldersMenu);

        profileMenu = menuBar->addMenu(tr("&Accounts"));
        profileMenu->setSeparatorsCollapsible(false);
        profileMenu->addAction(actionManageAccounts);

        helpMenu = menuBar->addMenu(tr("&Help"));
        helpMenu->setSeparatorsCollapsible(false);
        helpMenu->addAction(actionClearMetadata);
        #ifdef Q_OS_MAC
        helpMenu->addAction(actionAddToPATH);
        #endif
        helpMenu->addSeparator();
        helpMenu->addAction(actionAbout);
        helpMenu->addAction(actionOpenWiki);
        helpMenu->addAction(actionNewsMenuBar);
        helpMenu->addSeparator();
        if (!BuildConfig.BUG_TRACKER_URL.isEmpty())
            helpMenu->addAction(actionReportBug);
        if (!BuildConfig.MATRIX_URL.isEmpty())
            helpMenu->addAction(actionMATRIX);
        if (!BuildConfig.DISCORD_URL.isEmpty())
            helpMenu->addAction(actionDISCORD);
        if (!BuildConfig.SUBREDDIT_URL.isEmpty())
            helpMenu->addAction(actionREDDIT);
        if(BuildConfig.UPDATER_ENABLED)
        {
            helpMenu->addSeparator();
            helpMenu->addAction(actionCheckUpdate);
        }
        MainWindow->setMenuBar(menuBar);
    }

    void createMenuActions(MainWindow *MainWindow)
    {
        actionCloseWindow = TranslatedAction(MainWindow);
        actionCloseWindow->setObjectName(QStringLiteral("actionCloseWindow"));
        actionCloseWindow.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Close &Window"));
        actionCloseWindow.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Close the current window"));
        actionCloseWindow->setShortcut(QKeySequence::Close);
        connect(actionCloseWindow, &QAction::triggered, APPLICATION, &Application::closeCurrentWindow);
        all_actions.append(&actionCloseWindow);

        actionOpenWiki = TranslatedAction(MainWindow);
        actionOpenWiki->setObjectName(QStringLiteral("actionOpenWiki"));
        actionOpenWiki.setTextId(QT_TRANSLATE_NOOP("MainWindow", "%1 &Help"));
        actionOpenWiki.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the %1 wiki"));
        actionOpenWiki->setIcon(APPLICATION->getThemedIcon("help"));
        connect(actionOpenWiki, &QAction::triggered, MainWindow, &MainWindow::on_actionOpenWiki_triggered);
        all_actions.append(&actionOpenWiki);

        actionNewsMenuBar = TranslatedAction(MainWindow);
        actionNewsMenuBar->setObjectName(QStringLiteral("actionNewsMenuBar"));
        actionNewsMenuBar.setTextId(QT_TRANSLATE_NOOP("MainWindow", "%1 &News"));
        actionNewsMenuBar.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the %1 wiki"));
        actionNewsMenuBar->setIcon(APPLICATION->getThemedIcon("news"));
        connect(actionNewsMenuBar, &QAction::triggered, MainWindow, &MainWindow::on_actionMoreNews_triggered);
        all_actions.append(&actionNewsMenuBar);
    }

    // "Instance actions" are actions that require an instance to be selected (i.e. "new instance" is not here)
    // Actions that also require other conditions (e.g. a running instance) won't be changed.
    void setInstanceActionsEnabled(bool enabled)
    {
        actionEditInstance->setEnabled(enabled);
        actionChangeInstGroup->setEnabled(enabled);
        actionViewSelectedInstFolder->setEnabled(enabled);
        actionExportInstance->setEnabled(enabled);
        actionDeleteInstance->setEnabled(enabled);
        actionCopyInstance->setEnabled(enabled);
        actionCreateInstanceShortcut->setEnabled(enabled);
    }

    void createStatusBar(QMainWindow *MainWindow)
    {
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        MainWindow->setStatusBar(statusBar);
    }

    void createNewsToolbar(QMainWindow *MainWindow)
    {
        newsToolBar = TranslatedToolbar(MainWindow);
        newsToolBar->setObjectName(QStringLiteral("newsToolBar"));
        newsToolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
        newsToolBar->setIconSize(QSize(16, 16));
        newsToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        newsToolBar->setFloatable(false);
        newsToolBar->setWindowTitle(QT_TRANSLATE_NOOP("MainWindow", "News Toolbar"));

        actionMoreNews = TranslatedAction(MainWindow);
        actionMoreNews->setObjectName(QStringLiteral("actionMoreNews"));
        actionMoreNews->setIcon(APPLICATION->getThemedIcon("news"));
        actionMoreNews.setTextId(QT_TRANSLATE_NOOP("MainWindow", "More news..."));
        actionMoreNews.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the development blog to read more news about %1."));
        all_actions.append(&actionMoreNews);
        newsToolBar->addAction(actionMoreNews);

        all_toolbars.append(&newsToolBar);
        MainWindow->addToolBar(Qt::BottomToolBarArea, newsToolBar);
    }

    void createInstanceActions(QMainWindow *MainWindow)
    {
        // NOTE: not added to toolbar, but used for instance context menu (right click)
        actionChangeInstIcon = TranslatedAction(MainWindow);
        actionChangeInstIcon->setObjectName(QStringLiteral("actionChangeInstIcon"));
        actionChangeInstIcon->setIcon(QIcon(":/icons/instances/grass"));
        actionChangeInstIcon->setIconVisibleInMenu(true);
        actionChangeInstIcon.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Change Icon"));
        actionChangeInstIcon.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Change the selected instance's icon."));
        all_actions.append(&actionChangeInstIcon);

        changeIconButton = new LabeledToolButton(MainWindow);
        changeIconButton->setObjectName(QStringLiteral("changeIconButton"));
        changeIconButton->setIcon(APPLICATION->getThemedIcon("news"));
        changeIconButton->setToolTip(actionChangeInstIcon->toolTip());
        changeIconButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        // NOTE: not added to toolbar, but used for instance context menu (right click)
        actionRenameInstance = TranslatedAction(MainWindow);
        actionRenameInstance->setObjectName(QStringLiteral("actionRenameInstance"));
        actionRenameInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Rename"));
        actionRenameInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Rename the selected instance."));
        actionRenameInstance->setIcon(APPLICATION->getThemedIcon("rename"));
        all_actions.append(&actionRenameInstance);

        // the rename label is inside the rename tool button
        renameButton = new LabeledToolButton(MainWindow);
        renameButton->setObjectName(QStringLiteral("renameButton"));
        renameButton->setToolTip(actionRenameInstance->toolTip());
        renameButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        actionLaunchInstance = TranslatedAction(MainWindow);
        actionLaunchInstance->setObjectName(QStringLiteral("actionLaunchInstance"));
        actionLaunchInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Launch"));
        actionLaunchInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Launch the selected instance."));
        actionLaunchInstance->setIcon(APPLICATION->getThemedIcon("launch"));
        all_actions.append(&actionLaunchInstance);

        actionLaunchInstanceOffline = TranslatedAction(MainWindow);
        actionLaunchInstanceOffline->setObjectName(QStringLiteral("actionLaunchInstanceOffline"));
        actionLaunchInstanceOffline.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Launch &Offline"));
        actionLaunchInstanceOffline.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Launch the selected instance in offline mode."));
        all_actions.append(&actionLaunchInstanceOffline);

        actionLaunchInstanceDemo = TranslatedAction(MainWindow);
        actionLaunchInstanceDemo->setObjectName(QStringLiteral("actionLaunchInstanceDemo"));
        actionLaunchInstanceDemo.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Launch &Demo"));
        actionLaunchInstanceDemo.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Launch the selected instance in demo mode."));
        all_actions.append(&actionLaunchInstanceDemo);

        actionKillInstance = TranslatedAction(MainWindow);
        actionKillInstance->setObjectName(QStringLiteral("actionKillInstance"));
        actionKillInstance->setDisabled(true);
        actionKillInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Kill"));
        actionKillInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Kill the running instance"));
        actionKillInstance->setShortcut(QKeySequence(tr("Ctrl+K")));
        actionKillInstance->setIcon(APPLICATION->getThemedIcon("status-bad"));
        all_actions.append(&actionKillInstance);

        actionEditInstance = TranslatedAction(MainWindow);
        actionEditInstance->setObjectName(QStringLiteral("actionEditInstance"));
        actionEditInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Edit..."));
        actionEditInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Change the instance settings, mods and versions."));
        actionEditInstance->setShortcut(QKeySequence(tr("Ctrl+I")));
        actionEditInstance->setIcon(APPLICATION->getThemedIcon("settings-configure"));
        all_actions.append(&actionEditInstance);

        actionChangeInstGroup = TranslatedAction(MainWindow);
        actionChangeInstGroup->setObjectName(QStringLiteral("actionChangeInstGroup"));
        actionChangeInstGroup.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Change Group..."));
        actionChangeInstGroup.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Change the selected instance's group."));
        actionChangeInstGroup->setShortcut(QKeySequence(tr("Ctrl+G")));
        actionChangeInstGroup->setIcon(APPLICATION->getThemedIcon("tag"));
        all_actions.append(&actionChangeInstGroup);

        actionViewSelectedInstFolder = TranslatedAction(MainWindow);
        actionViewSelectedInstFolder->setObjectName(QStringLiteral("actionViewSelectedInstFolder"));
        actionViewSelectedInstFolder.setTextId(QT_TRANSLATE_NOOP("MainWindow", "&Folder"));
        actionViewSelectedInstFolder.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the selected instance's root folder in a file browser."));
        actionViewSelectedInstFolder->setIcon(APPLICATION->getThemedIcon("viewfolder"));
        all_actions.append(&actionViewSelectedInstFolder);

        actionExportInstance = TranslatedAction(MainWindow);
        actionExportInstance->setObjectName(QStringLiteral("actionExportInstance"));
        actionExportInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "E&xport..."));
        actionExportInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Export the selected instance as a zip file."));
        actionExportInstance->setShortcut(QKeySequence(tr("Ctrl+E")));
        actionExportInstance->setIcon(APPLICATION->getThemedIcon("export"));
        all_actions.append(&actionExportInstance);

        actionDeleteInstance = TranslatedAction(MainWindow);
        actionDeleteInstance->setObjectName(QStringLiteral("actionDeleteInstance"));
        actionDeleteInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Dele&te"));
        actionDeleteInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Delete the selected instance."));
        actionDeleteInstance->setShortcuts({QKeySequence(tr("Backspace")), QKeySequence::Delete});
        actionDeleteInstance->setAutoRepeat(false);
        actionDeleteInstance->setIcon(APPLICATION->getThemedIcon("delete"));
        all_actions.append(&actionDeleteInstance);

        actionCopyInstance = TranslatedAction(MainWindow);
        actionCopyInstance->setObjectName(QStringLiteral("actionCopyInstance"));
        actionCopyInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Cop&y..."));
        actionCopyInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Copy the selected instance."));
        actionCopyInstance->setShortcut(QKeySequence(tr("Ctrl+D")));
        actionCopyInstance->setIcon(APPLICATION->getThemedIcon("copy"));
        all_actions.append(&actionCopyInstance);

        actionCreateInstanceShortcut = TranslatedAction(MainWindow);
        actionCreateInstanceShortcut->setObjectName(QStringLiteral("actionCreateInstanceShortcut"));
        actionCreateInstanceShortcut.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Create Shortcut"));
        actionCreateInstanceShortcut.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Creates a shortcut on your desktop to launch the selected instance."));
        actionCreateInstanceShortcut->setIcon(APPLICATION->getThemedIcon("shortcut"));
        all_actions.append(&actionCreateInstanceShortcut);

        setInstanceActionsEnabled(false);
    }

    void createInstanceToolbar(QMainWindow *MainWindow)
    {
        instanceToolBar = TranslatedToolbar(MainWindow);
        instanceToolBar->setObjectName(QStringLiteral("instanceToolBar"));
        // disabled until we have an instance selected
        instanceToolBar->setEnabled(false);
        // Qt doesn't like vertical moving toolbars, so we have to force them...
        // See https://github.com/PolyMC/PolyMC/issues/493
        connect(instanceToolBar, &QToolBar::orientationChanged, [=](Qt::Orientation){ instanceToolBar->setOrientation(Qt::Vertical); });
        instanceToolBar->setAllowedAreas(Qt::LeftToolBarArea | Qt::RightToolBarArea);
        instanceToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        instanceToolBar->setIconSize(QSize(16, 16));

        instanceToolBar->setFloatable(false);
        instanceToolBar->setWindowTitle(QT_TRANSLATE_NOOP("MainWindow", "Instance Toolbar"));

        instanceToolBar->addWidget(changeIconButton);
        instanceToolBar->addWidget(renameButton);

        instanceToolBar->addSeparator();

        instanceToolBar->addAction(actionLaunchInstance);
        instanceToolBar->addAction(actionKillInstance);

        instanceToolBar->addSeparator();

        instanceToolBar->addAction(actionEditInstance);
        instanceToolBar->addAction(actionChangeInstGroup);

        instanceToolBar->addAction(actionViewSelectedInstFolder);

        instanceToolBar->addAction(actionExportInstance);
        instanceToolBar->addAction(actionCopyInstance);
        instanceToolBar->addAction(actionDeleteInstance);

        instanceToolBar->addAction(actionCreateInstanceShortcut); // TODO find better position for this

        QLayout * lay = instanceToolBar->layout();
        for(int i = 0; i < lay->count(); i++)
        {
            QLayoutItem * item = lay->itemAt(i);
            if (item->widget()->metaObject()->className() == QString("QToolButton"))
            {
                item->setAlignment(Qt::AlignLeft);
            }
        }

        all_toolbars.append(&instanceToolBar);
        MainWindow->addToolBar(Qt::RightToolBarArea, instanceToolBar);
    }

    void setupUi(MainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
        {
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        }
        MainWindow->resize(800, 600);
        MainWindow->setWindowIcon(APPLICATION->getThemedIcon("logo"));
        MainWindow->setWindowTitle(APPLICATION->applicationDisplayName());
#ifndef QT_NO_ACCESSIBILITY
        MainWindow->setAccessibleName(BuildConfig.LAUNCHER_DISPLAYNAME);
#endif

        createMainToolbarActions(MainWindow);
        createMenuActions(MainWindow);
        createInstanceActions(MainWindow);

        createMenuBar(MainWindow);

        createMainToolbar(MainWindow);

        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        horizontalLayout = new QHBoxLayout(centralWidget);
        horizontalLayout->setSpacing(0);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        MainWindow->setCentralWidget(centralWidget);

        createStatusBar(MainWindow);
        createNewsToolbar(MainWindow);
        createInstanceToolbar(MainWindow);

        MainWindow->updateToolsMenu();
        MainWindow->updateThemeMenu();

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(MainWindow *MainWindow)
    {
        // all the actions
        for(auto * item: all_actions)
        {
            item->retranslate();
        }
        for(auto * item: all_toolbars)
        {
            item->retranslate();
        }
        for(auto * item: all_toolbuttons)
        {
            item->retranslate();
        }
        // submenu buttons
        foldersMenuButton->setText(tr("Folders"));
        helpMenuButton->setText(tr("Help"));

        // playtime counter
        if (MainWindow->m_statusCenter)
        {
            MainWindow->updateStatusCenter();
        }
    } // retranslateUi
};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new MainWindow::Ui)
{
    ui->setupUi(this);

    // OSX magic.
    setUnifiedTitleAndToolBarOnMac(true);

    // Global shortcuts
    {
        // FIXME: This is kinda weird. and bad. We need some kind of managed shutdown.
        auto q = new QShortcut(QKeySequence::Quit, this);
        connect(q, SIGNAL(activated()), qApp, SLOT(quit()));
    }

    // Konami Code
    {
        secretEventFilter = new KonamiCode(this);
        connect(secretEventFilter, &KonamiCode::triggered, this, &MainWindow::konamiTriggered);
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

    // Create the instance list widget
    {
        view = new InstanceView(ui->centralWidget);

        view->setSelectionMode(QAbstractItemView::SingleSelection);
        // FIXME: leaks ListViewDelegate
        view->setItemDelegate(new ListViewDelegate(this));
        view->setFrameShape(QFrame::NoFrame);
        // do not show ugly blue border on the mac
        view->setAttribute(Qt::WA_MacShowFocusRect, false);

        view->installEventFilter(this);
        view->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(view, &QWidget::customContextMenuRequested, this, &MainWindow::showInstanceContextMenu);
        connect(view, &InstanceView::droppedURLs, this, &MainWindow::droppedURLs, Qt::QueuedConnection);

        proxymodel = new InstanceProxyModel(this);
        proxymodel->setSourceModel(APPLICATION->instances().get());
        proxymodel->sort(0);
        connect(proxymodel, &InstanceProxyModel::dataChanged, this, &MainWindow::instanceDataChanged);

        view->setModel(proxymodel);
        view->setSourceOfGroupCollapseStatus([](const QString & groupName)->bool {
            return APPLICATION->instances()->isGroupCollapsed(groupName);
        });
        connect(view, &InstanceView::groupStateChanged, APPLICATION->instances().get(), &InstanceList::on_GroupStateChanged);
        ui->horizontalLayout->addWidget(view);
    }
    // The cat background
    {
        bool cat_enable = APPLICATION->settings()->get("TheCat").toBool();
        ui->actionCAT->setChecked(cat_enable);
        // NOTE: calling the operator like that is an ugly hack to appease ancient gcc...
        connect(ui->actionCAT.operator->(), SIGNAL(toggled(bool)), SLOT(onCatToggled(bool)));
        setCatBackground(cat_enable);
    }

    // Lock toolbars
    {
        bool toolbarsLocked = APPLICATION->settings()->get("ToolbarsLocked").toBool();
        ui->actionLockToolbars->setChecked(toolbarsLocked);
        connect(ui->actionLockToolbars, &QAction::toggled, this, &MainWindow::lockToolbars);
        lockToolbars(toolbarsLocked);
    }
    // start instance when double-clicked
    connect(view, &InstanceView::activated, this, &MainWindow::instanceActivated);

    // track the selection -- update the instance toolbar
    connect(view->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::instanceChanged);

    // track icon changes and update the toolbar!
    connect(APPLICATION->icons().get(), &IconList::iconUpdated, this, &MainWindow::iconUpdated);

    // model reset -> selection is invalid. All the instance pointers are wrong.
    connect(APPLICATION->instances().get(), &InstanceList::dataIsInvalid, this, &MainWindow::selectionBad);

    // handle newly added instances
    connect(APPLICATION->instances().get(), &InstanceList::instanceSelectRequest, this, &MainWindow::instanceSelectRequest);

    // When the global settings page closes, we want to know about it and update our state
    connect(APPLICATION, &Application::globalSettingsClosed, this, &MainWindow::globalSettingsClosed);

    m_statusLeft = new QLabel(tr("No instance selected"), this);
    m_statusCenter = new QLabel(tr("Total playtime: 0s"), this);
    statusBar()->addPermanentWidget(m_statusLeft, 1);
    statusBar()->addPermanentWidget(m_statusCenter, 0);

    // Add "manage accounts" button, right align
    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->mainToolBar->addWidget(spacer);

    accountMenu = new QMenu(this);
    // Use undocumented property... https://stackoverflow.com/questions/7121718/create-a-scrollbar-in-a-submenu-qt
    accountMenu->setStyleSheet("QMenu { menu-scrollable: 1; }");

    repopulateAccountsMenu();

    accountMenuButton = new QToolButton(this);
    accountMenuButton->setMenu(accountMenu);
    accountMenuButton->setPopupMode(QToolButton::InstantPopup);
    accountMenuButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    accountMenuButton->setIcon(APPLICATION->getThemedIcon("noaccount"));

    QWidgetAction *accountMenuButtonAction = new QWidgetAction(this);
    accountMenuButtonAction->setDefaultWidget(accountMenuButton);

    ui->mainToolBar->addAction(accountMenuButtonAction);

    // Update the menu when the active account changes.
    // Shouldn't have to use lambdas here like this, but if I don't, the compiler throws a fit.
    // Template hell sucks...
    connect(
        APPLICATION->accounts().get(),
        &AccountList::defaultAccountChanged,
        [this] {
            defaultAccountChanged();
        }
    );
    connect(
        APPLICATION->accounts().get(),
        &AccountList::listChanged,
        [this]
        {
            repopulateAccountsMenu();
        }
    );

    // Show initial account
    defaultAccountChanged();

    // TODO: refresh accounts here?
    // auto accounts = APPLICATION->accounts();

    // load the news
    {
        m_newsChecker->reloadNews();
        updateNewsLabel();
    }


    if(BuildConfig.UPDATER_ENABLED)
    {
        bool updatesAllowed = APPLICATION->updatesAreAllowed();
        updatesAllowedChanged(updatesAllowed);

        // NOTE: calling the operator like that is an ugly hack to appease ancient gcc...
        connect(ui->actionCheckUpdate.operator->(), &QAction::triggered, this, &MainWindow::checkForUpdates);

        // set up the updater object.
        auto updater = APPLICATION->updateChecker();
        connect(updater.get(), &UpdateChecker::updateAvailable, this, &MainWindow::updateAvailable);
        connect(updater.get(), &UpdateChecker::noUpdateFound, this, &MainWindow::updateNotAvailable);
        // if automatic update checks are allowed, start one.
        if (APPLICATION->settings()->get("AutoUpdate").toBool() && updatesAllowed)
        {
            updater->checkForUpdate(APPLICATION->settings()->get("UpdateChannel").toString(), false);
        }

        if (APPLICATION->updateChecker()->getExternalUpdater())
        {
            connect(APPLICATION->updateChecker()->getExternalUpdater(),
                    &ExternalUpdater::canCheckForUpdatesChanged,
                    this,
                    &MainWindow::updatesAllowedChanged);
        }
    }

    connect(ui->actionUndoTrashInstance.operator->(), &QAction::triggered, this, &MainWindow::undoTrashInstance);

    setSelectedInstanceById(APPLICATION->settings()->get("SelectedInstance").toString());

    // removing this looks stupid
    view->setFocus();

    retranslateUi();
}

// macOS always has a native menu bar, so these fixes are not applicable
// Other systems may or may not have a native menu bar (most do not - it seems like only Ubuntu Unity does)
#ifndef Q_OS_MAC
void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if(event->key()==Qt::Key_Alt && !APPLICATION->settings()->get("MenuBarInsteadOfToolBar").toBool())
        ui->menuBar->setVisible(!ui->menuBar->isVisible());
    else
        QMainWindow::keyReleaseEvent(event);
}
#endif

void MainWindow::retranslateUi()
{
    auto accounts = APPLICATION->accounts();
    MinecraftAccountPtr defaultAccount = accounts->defaultAccount();
    if(defaultAccount) {
        auto profileLabel = profileInUseFilter(defaultAccount->profileName(), defaultAccount->isInUse());
        accountMenuButton->setText(profileLabel);
    }
    else {
        accountMenuButton->setText(tr("Accounts"));
    }

    if (m_selectedInstance) {
        m_statusLeft->setText(m_selectedInstance->getStatusbarDescription());
    } else {
        m_statusLeft->setText(tr("No instance selected"));
    }

    ui->retranslateUi(this);
}

MainWindow::~MainWindow()
{
}

QMenu * MainWindow::createPopupMenu()
{
    QMenu* filteredMenu = QMainWindow::createPopupMenu();
    filteredMenu->removeAction( ui->mainToolBar->toggleViewAction() );

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


void MainWindow::konamiTriggered()
{
    qDebug() << "Super Secret Mode ACTIVATED!";
}

void MainWindow::showInstanceContextMenu(const QPoint &pos)
{
    QList<QAction *> actions;

    QAction *actionSep = new QAction("", this);
    actionSep->setSeparator(true);

    bool onInstance = view->indexAt(pos).isValid();
    if (onInstance)
    {
        actions = ui->instanceToolBar->actions();

        // replace the change icon widget with an actual action
        actions.replace(0, ui->actionChangeInstIcon);

        // replace the rename widget with an actual action
        actions.replace(1, ui->actionRenameInstance);

        // add header
        actions.prepend(actionSep);
        QAction *actionVoid = new QAction(m_selectedInstance->name(), this);
        actionVoid->setEnabled(false);
        actions.prepend(actionVoid);
    }
    else
    {
        auto group = view->groupNameAt(pos);

        QAction *actionVoid = new QAction(BuildConfig.LAUNCHER_DISPLAYNAME, this);
        actionVoid->setEnabled(false);

        QAction *actionCreateInstance = new QAction(tr("Create instance"), this);
        actionCreateInstance->setToolTip(ui->actionAddInstance->toolTip());
        if(!group.isNull())
        {
            QVariantMap data;
            data["group"] = group;
            actionCreateInstance->setData(data);
        }

        connect(actionCreateInstance, SIGNAL(triggered(bool)), SLOT(on_actionAddInstance_triggered()));

        actions.prepend(actionSep);
        actions.prepend(actionVoid);
        actions.append(actionCreateInstance);
        if(!group.isNull())
        {
            QAction *actionDeleteGroup = new QAction(tr("Delete group '%1'").arg(group), this);
            QVariantMap data;
            data["group"] = group;
            actionDeleteGroup->setData(data);
            connect(actionDeleteGroup, SIGNAL(triggered(bool)), SLOT(deleteGroup()));
            actions.append(actionDeleteGroup);
        }
    }
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

void MainWindow::updateToolsMenu()
{
    QToolButton *launchButton = dynamic_cast<QToolButton*>(ui->instanceToolBar->widgetForAction(ui->actionLaunchInstance));

    bool currentInstanceRunning = m_selectedInstance && m_selectedInstance->isRunning();

    ui->actionLaunchInstance->setDisabled(!m_selectedInstance || currentInstanceRunning);
    ui->actionLaunchInstanceOffline->setDisabled(!m_selectedInstance || currentInstanceRunning);
    ui->actionLaunchInstanceDemo->setDisabled(!m_selectedInstance || currentInstanceRunning);

    QMenu *launchMenu = ui->actionLaunchInstance->menu();
    launchButton->setPopupMode(QToolButton::MenuButtonPopup);
    if (launchMenu)
    {
        launchMenu->clear();
    }
    else
    {
        launchMenu = new QMenu(this);
    }

    QAction *normalLaunch = launchMenu->addAction(tr("Launch"));
    normalLaunch->setShortcut(QKeySequence::Open);
    QAction *normalLaunchOffline = launchMenu->addAction(tr("Launch Offline"));
    normalLaunchOffline->setShortcut(QKeySequence(tr("Ctrl+Shift+O")));
    QAction *normalLaunchDemo = launchMenu->addAction(tr("Launch Demo"));
    normalLaunchDemo->setShortcut(QKeySequence(tr("Ctrl+Alt+O")));
    if (m_selectedInstance)
    {
        normalLaunch->setEnabled(m_selectedInstance->canLaunch());
        normalLaunchOffline->setEnabled(m_selectedInstance->canLaunch());
        normalLaunchDemo->setEnabled(m_selectedInstance->canLaunch());

        connect(normalLaunch, &QAction::triggered, [this]() {
            APPLICATION->launch(m_selectedInstance, true, false);
        });
        connect(normalLaunchOffline, &QAction::triggered, [this]() {
            APPLICATION->launch(m_selectedInstance, false, false);
        });
        connect(normalLaunchDemo, &QAction::triggered, [this]() {
            APPLICATION->launch(m_selectedInstance, false, true);
        });
    }
    else
    {
        normalLaunch->setDisabled(true);
        normalLaunchOffline->setDisabled(true);
        normalLaunchDemo->setDisabled(true);
    }

    // Disable demo-mode if not available.
    auto instance = dynamic_cast<MinecraftInstance*>(m_selectedInstance.get());
    if (instance) {
        normalLaunchDemo->setEnabled(instance->supportsDemo());
    }

    QString profilersTitle = tr("Profilers");
    launchMenu->addSeparator()->setText(profilersTitle);
    for (auto profiler : APPLICATION->profilers().values())
    {
        QAction *profilerAction = launchMenu->addAction(profiler->name());
        QAction *profilerOfflineAction = launchMenu->addAction(tr("%1 Offline").arg(profiler->name()));
        QString error;
        if (!profiler->check(&error))
        {
            profilerAction->setDisabled(true);
            profilerOfflineAction->setDisabled(true);
            QString profilerToolTip = tr("Profiler not setup correctly. Go into settings, \"External Tools\".");
            profilerAction->setToolTip(profilerToolTip);
            profilerOfflineAction->setToolTip(profilerToolTip);
        }
        else if (m_selectedInstance)
        {
            profilerAction->setEnabled(m_selectedInstance->canLaunch());
            profilerOfflineAction->setEnabled(m_selectedInstance->canLaunch());

            connect(profilerAction, &QAction::triggered, [this, profiler]()
                    {
                        APPLICATION->launch(m_selectedInstance, true, false, profiler.get());
                    });
            connect(profilerOfflineAction, &QAction::triggered, [this, profiler]()
                    {
                        APPLICATION->launch(m_selectedInstance, false, false, profiler.get());
                    });
        }
        else
        {
            profilerAction->setDisabled(true);
            profilerOfflineAction->setDisabled(true);
        }
    }
    ui->actionLaunchInstance->setMenu(launchMenu);
}

void MainWindow::updateThemeMenu()
{
    QMenu *themeMenu = ui->actionChangeTheme->menu();

    if (themeMenu) {
        themeMenu->clear();
    } else {
        themeMenu = new QMenu(this);
    }

    auto themes = APPLICATION->getValidApplicationThemes();

    QActionGroup* themesGroup = new QActionGroup( this );

    for (auto* theme : themes) {
        QAction * themeAction = themeMenu->addAction(theme->name());

        themeAction->setCheckable(true);
        if (APPLICATION->settings()->get("ApplicationTheme").toString() == theme->id()) {
            themeAction->setChecked(true);
        }
        themeAction->setActionGroup(themesGroup);

        connect(themeAction, &QAction::triggered, [theme]() {
            APPLICATION->setApplicationTheme(theme->id(),false);
            APPLICATION->settings()->set("ApplicationTheme", theme->id());
        });
    }

    ui->actionChangeTheme->setMenu(themeMenu);
}

void MainWindow::repopulateAccountsMenu()
{
    accountMenu->clear();
    ui->profileMenu->clear();

    auto accounts = APPLICATION->accounts();
    MinecraftAccountPtr defaultAccount = accounts->defaultAccount();

    QString active_profileId = "";
    if (defaultAccount)
    {
        // this can be called before accountMenuButton exists
        if (accountMenuButton)
        {
            auto profileLabel = profileInUseFilter(defaultAccount->profileName(), defaultAccount->isInUse());
            accountMenuButton->setText(profileLabel);
        }
    }

    if (accounts->count() <= 0)
    {
        ui->all_actions.removeAll(&ui->actionNoAccountsAdded);
        ui->actionNoAccountsAdded = TranslatedAction(this);
        ui->actionNoAccountsAdded->setObjectName(QStringLiteral("actionNoAccountsAdded"));
        ui->actionNoAccountsAdded.setTextId(QT_TRANSLATE_NOOP("MainWindow", "No accounts added!"));
        ui->actionNoAccountsAdded->setEnabled(false);
        accountMenu->addAction(ui->actionNoAccountsAdded);
        ui->profileMenu->addAction(ui->actionNoAccountsAdded);
        ui->all_actions.append(&ui->actionNoAccountsAdded);
    }
    else
    {
        // TODO: Nicer way to iterate?
        for (int i = 0; i < accounts->count(); i++)
        {
            MinecraftAccountPtr account = accounts->at(i);
            auto profileLabel = profileInUseFilter(account->profileName(), account->isInUse());
            QAction *action = new QAction(profileLabel, this);
            action->setData(i);
            action->setCheckable(true);
            if (defaultAccount == account)
            {
                action->setChecked(true);
            }

            auto face = account->getFace();
            if(!face.isNull()) {
                action->setIcon(face);
            }
            else {
                action->setIcon(APPLICATION->getThemedIcon("noaccount"));
            }

            const int highestNumberKey = 9;
            if(i<highestNumberKey)
            {
                action->setShortcut(QKeySequence(tr("Ctrl+%1").arg(i + 1)));
            }

            accountMenu->addAction(action);
            ui->profileMenu->addAction(action);
            connect(action, SIGNAL(triggered(bool)), SLOT(changeActiveAccount()));
        }
    }

    accountMenu->addSeparator();
    ui->profileMenu->addSeparator();

    ui->all_actions.removeAll(&ui->actionNoDefaultAccount);
    ui->actionNoDefaultAccount = TranslatedAction(this);
    ui->actionNoDefaultAccount->setObjectName(QStringLiteral("actionNoDefaultAccount"));
    ui->actionNoDefaultAccount.setTextId(QT_TRANSLATE_NOOP("MainWindow", "No Default Account"));
    ui->actionNoDefaultAccount->setCheckable(true);
    ui->actionNoDefaultAccount->setIcon(APPLICATION->getThemedIcon("noaccount"));
    ui->actionNoDefaultAccount->setData(-1);
    ui->actionNoDefaultAccount->setShortcut(QKeySequence(tr("Ctrl+0")));
    if (!defaultAccount) {
        ui->actionNoDefaultAccount->setChecked(true);
    }

    accountMenu->addAction(ui->actionNoDefaultAccount);
    ui->profileMenu->addAction(ui->actionNoDefaultAccount);
    connect(ui->actionNoDefaultAccount, SIGNAL(triggered(bool)), SLOT(changeActiveAccount()));
    ui->all_actions.append(&ui->actionNoDefaultAccount);
    ui->actionNoDefaultAccount.retranslate();

    accountMenu->addSeparator();
    ui->profileMenu->addSeparator();
    accountMenu->addAction(ui->actionManageAccounts);
    ui->profileMenu->addAction(ui->actionManageAccounts);
}

void MainWindow::updatesAllowedChanged(bool allowed)
{
    if(!BuildConfig.UPDATER_ENABLED)
    {
        return;
    }
    ui->actionCheckUpdate->setEnabled(allowed);
}

/*
 * Assumes the sender is a QAction
 */
void MainWindow::changeActiveAccount()
{
    QAction *sAction = (QAction *)sender();

    // Profile's associated Mojang username
    if (sAction->data().type() != QVariant::Type::Int)
        return;

    QVariant data = sAction->data();
    bool valid = false;
    int index = data.toInt(&valid);
    if(!valid) {
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
    if (account && account->profileName() != "")
    {
        auto profileLabel = profileInUseFilter(account->profileName(), account->isInUse());
        accountMenuButton->setText(profileLabel);
        auto face = account->getFace();
        if(face.isNull()) {
            accountMenuButton->setIcon(APPLICATION->getThemedIcon("noaccount"));
        }
        else {
            accountMenuButton->setIcon(face);
        }
        return;
    }

    // Set the icon to the "no account" icon.
    accountMenuButton->setIcon(APPLICATION->getThemedIcon("noaccount"));
    accountMenuButton->setText(tr("Accounts"));
}

bool MainWindow::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == view)
    {
        if (ev->type() == QEvent::KeyPress)
        {
            secretEventFilter->input(ev);
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
            switch (keyEvent->key())
            {
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
            case Qt::Key_F2:
                on_actionRenameInstance_triggered();
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
    if (m_newsChecker->isLoadingNews())
    {
        newsLabel->setText(tr("Loading news..."));
        newsLabel->setEnabled(false);
        ui->actionMoreNews->setVisible(false);
    }
    else
    {
        QList<NewsEntryPtr> entries = m_newsChecker->getNewsEntries();
        if (entries.length() > 0)
        {
            newsLabel->setText(entries[0]->title);
            newsLabel->setEnabled(true);
            ui->actionMoreNews->setVisible(true);
        }
        else
        {
            newsLabel->setText(tr("No news available."));
            newsLabel->setEnabled(false);
            ui->actionMoreNews->setVisible(false);
        }
    }
}

void MainWindow::updateAvailable(GoUpdate::Status status)
{
    if(!APPLICATION->updatesAreAllowed())
    {
        updateNotAvailable();
        return;
    }
    UpdateDialog dlg(true, this);
    UpdateAction action = (UpdateAction)dlg.exec();
    switch (action)
    {
    case UPDATE_LATER:
        qDebug() << "Update will be installed later.";
        break;
    case UPDATE_NOW:
        downloadUpdates(status);
        break;
    }
}

void MainWindow::updateNotAvailable()
{
    UpdateDialog dlg(false, this);
    dlg.exec();
}

QList<int> stringToIntList(const QString &string)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QStringList split = string.split(',', Qt::SkipEmptyParts);
#else
    QStringList split = string.split(',', QString::SkipEmptyParts);
#endif
    QList<int> out;
    for (int i = 0; i < split.size(); ++i)
    {
        out.append(split.at(i).toInt());
    }
    return out;
}
QString intListToString(const QList<int> &list)
{
    QStringList slist;
    for (int i = 0; i < list.size(); ++i)
    {
        slist.append(QString::number(list.at(i)));
    }
    return slist.join(',');
}

void MainWindow::downloadUpdates(GoUpdate::Status status)
{
    if(!APPLICATION->updatesAreAllowed())
    {
        return;
    }
    qDebug() << "Downloading updates.";
    ProgressDialog updateDlg(this);
    status.rootPath = APPLICATION->root();

    auto dlPath = FS::PathCombine(APPLICATION->root(), "update", "XXXXXX");
    if (!FS::ensureFilePathExists(dlPath))
    {
        CustomMessageBox::selectable(this, tr("Error"), tr("Couldn't create folder for update downloads:\n%1").arg(dlPath), QMessageBox::Warning)->show();
    }
    GoUpdate::DownloadTask updateTask(APPLICATION->network(), status, dlPath, &updateDlg);
    // If the task succeeds, install the updates.
    if (updateDlg.execWithTask(&updateTask))
    {
        /**
         * NOTE: This disables launching instances until the update either succeeds (and this process exits)
         * or the update fails (and the control leaves this scope).
         */
        APPLICATION->updateIsRunning(true);
        UpdateController update(this, APPLICATION->root(), updateTask.updateFilesDir(), updateTask.operations());
        update.installUpdates();
        APPLICATION->updateIsRunning(false);
    }
    else
    {
        CustomMessageBox::selectable(this, tr("Error"), updateTask.failReason(), QMessageBox::Warning)->show();
    }
}

void MainWindow::onCatToggled(bool state)
{
    setCatBackground(state);
    APPLICATION->settings()->set("TheCat", state);
}

namespace {
template <typename T>
T non_stupid_abs(T in)
{
    if (in < 0)
        return -in;
    return in;
}
}

void MainWindow::setCatBackground(bool enabled)
{
    if (enabled)
    {
        QDateTime now = QDateTime::currentDateTime();
        QDateTime birthday(QDate(now.date().year(), 11, 30), QTime(0, 0));
        QDateTime xmas(QDate(now.date().year(), 12, 25), QTime(0, 0));
        QDateTime halloween(QDate(now.date().year(), 10, 31), QTime(0, 0));
        QString cat = APPLICATION->settings()->get("BackgroundCat").toString();
        if (non_stupid_abs(now.daysTo(xmas)) <= 4) {
            cat += "-xmas";
        } else if (non_stupid_abs(now.daysTo(halloween)) <= 4) {
            cat += "-spooky";
        } else if (non_stupid_abs(now.daysTo(birthday)) <= 12) {
            cat += "-bday";
        }
        view->setStyleSheet(QString(R"(
InstanceView
{
    background-image: url(:/backgrounds/%1);
    background-attachment: fixed;
    background-clip: padding;
    background-position: bottom right;
    background-repeat: none;
    background-color:palette(base);
})")
                                .arg(cat));
    }
    else
    {
        view->setStyleSheet(QString());
    }
}

void MainWindow::runModalTask(Task *task)
{
    connect(task, &Task::failed, [this](QString reason)
        {
            CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show();
        });
    connect(task, &Task::succeeded, [this, task]()
        {
            QStringList warnings = task->warnings();
            if(warnings.count())
            {
                CustomMessageBox::selectable(this, tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->show();
            }
        });
    connect(task, &Task::aborted, [this]
        {
            CustomMessageBox::selectable(this, tr("Task aborted"), tr("The task has been aborted by the user."), QMessageBox::Information)->show();
        });
    ProgressDialog loadDialog(this);
    loadDialog.setSkipButton(true, tr("Abort"));
    loadDialog.execWithTask(task);
}

void MainWindow::instanceFromInstanceTask(InstanceTask *rawTask)
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
    view->updateGeometries();
    setSelectedInstanceById(inst->id());
    if (APPLICATION->accounts()->anyAccountIsValid())
    {
        ProgressDialog loadDialog(this);
        auto update = inst->createUpdateTask(Net::Mode::Online);
        connect(update.get(), &Task::failed, [this](QString reason)
                {
                    QString error = QString("Instance load failed: %1").arg(reason);
                    CustomMessageBox::selectable(this, tr("Error"), error, QMessageBox::Warning)->show();
                });
        if(update)
        {
            loadDialog.setSkipButton(true, tr("Abort"));
            loadDialog.execWithTask(update.get());
        }
    }
    else
    {
        CustomMessageBox::selectable(
            this,
            tr("Error"),
            tr("The launcher cannot download Minecraft or update instances unless you have at least "
                "one account added.\nPlease add your Mojang or Minecraft account."),
            QMessageBox::Warning
        )->show();
    }
}

void MainWindow::addInstance(QString url)
{
    QString groupName;
    do
    {
        QObject* obj = sender();
        if(!obj)
            break;
        QAction *action = qobject_cast<QAction *>(obj);
        if(!action)
            break;
        auto map = action->data().toMap();
        if(!map.contains("group"))
            break;
        groupName = map["group"].toString();
    } while(0);

    if(groupName.isEmpty())
    {
        groupName = APPLICATION->settings()->get("LastUsedGroupForNewInstance").toString();
    }

    NewInstanceDialog newInstDlg(groupName, url, this);
    if (!newInstDlg.exec())
        return;

    APPLICATION->settings()->set("LastUsedGroupForNewInstance", newInstDlg.instGroup());

    InstanceTask * creationTask = newInstDlg.extractTask();
    if(creationTask)
    {
        instanceFromInstanceTask(creationTask);
    }
}

void MainWindow::on_actionAddInstance_triggered()
{
    addInstance();
}

void MainWindow::droppedURLs(QList<QUrl> urls)
{
    // NOTE: This loop only processes one dropped file!
    for (auto& url : urls) {
        // The isLocalFile() check below doesn't work as intended without an explicit scheme.
        if (url.scheme().isEmpty())
            url.setScheme("file");

        if (!url.isLocalFile()) {  // probably instance/modpack
            addInstance(url.toString());
            break;
        }

        auto localFileName = url.toLocalFile();
        QFileInfo localFileInfo(localFileName);

        bool isResourcePack = ResourcePackUtils::validate(localFileInfo);
        bool isTexturePack = TexturePackUtils::validate(localFileInfo);

        if (!isResourcePack && !isTexturePack) {  // probably instance/modpack
            addInstance(localFileName);
            break;
        }

        ImportResourcePackDialog dlg(this);

        if (dlg.exec() != QDialog::Accepted)
            break;

        qDebug() << "Adding resource/texture pack" << localFileName << "to" << dlg.selectedInstanceKey;

        auto inst = APPLICATION->instances()->getInstanceById(dlg.selectedInstanceKey);
        auto minecraftInst = std::dynamic_pointer_cast<MinecraftInstance>(inst);
        if (isResourcePack)
            minecraftInst->resourcePackList()->installResource(localFileName);
        else if (isTexturePack)
            minecraftInst->texturePackList()->installResource(localFileName);
        break;
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
    if (dlg.result() == QDialog::Accepted)
    {
        m_selectedInstance->setIconKey(dlg.selectedIconKey);
        auto icon = APPLICATION->icons()->getIcon(dlg.selectedIconKey);
        ui->actionChangeInstIcon->setIcon(icon);
        ui->changeIconButton->setIcon(icon);
    }
}

void MainWindow::iconUpdated(QString icon)
{
    if (icon == m_currentInstIcon)
    {
        auto icon = APPLICATION->icons()->getIcon(m_currentInstIcon);
        ui->actionChangeInstIcon->setIcon(icon);
        ui->changeIconButton->setIcon(icon);
    }
}

void MainWindow::updateInstanceToolIcon(QString new_icon)
{
    m_currentInstIcon = new_icon;
    auto icon = APPLICATION->icons()->getIcon(m_currentInstIcon);
    ui->actionChangeInstIcon->setIcon(icon);
    ui->changeIconButton->setIcon(icon);
}

void MainWindow::setSelectedInstanceById(const QString &id)
{
    if (id.isNull())
        return;
    const QModelIndex index = APPLICATION->instances()->getInstanceIndexById(id);
    if (index.isValid())
    {
        QModelIndex selectionIndex = proxymodel->mapFromSource(index);
        view->selectionModel()->setCurrentIndex(selectionIndex, QItemSelectionModel::ClearAndSelect);
        updateStatusCenter();
    }
}

void MainWindow::on_actionChangeInstGroup_triggered()
{
    if (!m_selectedInstance)
        return;

    bool ok = false;
    InstanceId instId = m_selectedInstance->id();
    QString name(APPLICATION->instances()->getInstanceGroup(instId));
    auto groups = APPLICATION->instances()->getGroups();
    groups.insert(0, "");
    groups.sort(Qt::CaseInsensitive);
    int foo = groups.indexOf(name);

    name = QInputDialog::getItem(this, tr("Group name"), tr("Enter a new group name."), groups, foo, true, &ok);
    name = name.simplified();
    if (ok)
    {
        APPLICATION->instances()->setInstanceGroup(instId, name);
    }
}

void MainWindow::deleteGroup()
{
    QObject* obj = sender();
    if(!obj)
        return;
    QAction *action = qobject_cast<QAction *>(obj);
    if(!action)
        return;
    auto map = action->data().toMap();
    if(!map.contains("group"))
        return;
    QString groupName = map["group"].toString();
    if(!groupName.isEmpty())
    {
        auto reply = QMessageBox::question(this, tr("Delete group"), tr("Are you sure you want to delete the group %1?")
            .arg(groupName), QMessageBox::Yes | QMessageBox::No);
        if(reply == QMessageBox::Yes)
        {
            APPLICATION->instances()->deleteGroup(groupName);
        }
    }
}

void MainWindow::undoTrashInstance()
{
    APPLICATION->instances()->undoTrashInstance();
    ui->actionUndoTrashInstance->setEnabled(APPLICATION->instances()->trashedSomething());
}

void MainWindow::on_actionViewInstanceFolder_triggered()
{
    QString str = APPLICATION->settings()->get("InstanceDir").toString();
    DesktopServices::openDirectory(str);
}

void MainWindow::refreshInstances()
{
    APPLICATION->instances()->loadList();
}

void MainWindow::on_actionViewCentralModsFolder_triggered()
{
    DesktopServices::openDirectory(APPLICATION->settings()->get("CentralModsDir").toString(), true);
}

void MainWindow::checkForUpdates()
{
    if(BuildConfig.UPDATER_ENABLED)
    {
        auto updater = APPLICATION->updateChecker();
        updater->checkForUpdate(APPLICATION->settings()->get("UpdateChannel").toString(), true);
    }
    else
    {
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
    proxymodel->invalidate();
    proxymodel->sort(0);
    updateMainToolBar();
    updateToolsMenu();
    updateThemeMenu();
    updateStatusCenter();
    // This needs to be done to prevent UI elements disappearing in the event the config is changed
    // but Prism Launcher exits abnormally, causing the window state to never be saved:
    APPLICATION->settings()->set("MainWindowState", saveState().toBase64());
    update();
}

void MainWindow::on_actionEditInstance_triggered()
{
    APPLICATION->showInstanceWindow(m_selectedInstance);
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

    auto response =
        CustomMessageBox::selectable(this, tr("CAREFUL!"),
                                     tr("About to delete: %1\nThis may be permanent and will completely delete the instance.\n\nAre you sure?")
                                         .arg(m_selectedInstance->name()),
                                     QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
            ->exec();

    if (response == QMessageBox::Yes) {
        if (APPLICATION->instances()->trashInstance(id)) {
            ui->actionUndoTrashInstance->setEnabled(APPLICATION->instances()->trashedSomething());
            return;
        }

        APPLICATION->instances()->deleteInstance(id);
    }
}

void MainWindow::on_actionExportInstance_triggered()
{
    if (m_selectedInstance)
    {
        ExportInstanceDialog dlg(m_selectedInstance, this);
        dlg.exec();
    }
}

void MainWindow::on_actionRenameInstance_triggered()
{
    if (m_selectedInstance)
    {
        view->edit(view->currentIndex());
    }
}

void MainWindow::on_actionViewSelectedInstFolder_triggered()
{
    if (m_selectedInstance)
    {
        QString str = m_selectedInstance->instanceRoot();
        DesktopServices::openDirectory(QDir(str).absolutePath());
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Save the window state and geometry.
    APPLICATION->settings()->set("MainWindowState", saveState().toBase64());
    APPLICATION->settings()->set("MainWindowGeometry", saveGeometry().toBase64());
    event->accept();
    emit isClosing();
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        retranslateUi();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::instanceActivated(QModelIndex index)
{
    if (!index.isValid())
        return;
    QString id = index.data(InstanceList::InstanceIDRole).toString();
    InstancePtr inst = APPLICATION->instances()->getInstanceById(id);
    if (!inst)
        return;

    activateInstance(inst);
}

void MainWindow::on_actionLaunchInstance_triggered()
{
    if(m_selectedInstance && !m_selectedInstance->isRunning())
    {
        APPLICATION->launch(m_selectedInstance);
    }
}

void MainWindow::activateInstance(InstancePtr instance)
{
    APPLICATION->launch(instance);
}

void MainWindow::on_actionLaunchInstanceOffline_triggered()
{
    if (m_selectedInstance)
    {
        APPLICATION->launch(m_selectedInstance, false);
    }
}

void MainWindow::on_actionLaunchInstanceDemo_triggered()
{
    if (m_selectedInstance)
    {
        APPLICATION->launch(m_selectedInstance, false, true);
    }
}

void MainWindow::on_actionKillInstance_triggered()
{
    if(m_selectedInstance && m_selectedInstance->isRunning())
    {
        APPLICATION->kill(m_selectedInstance);
    }
}

void MainWindow::on_actionCreateInstanceShortcut_triggered()
{
    if (m_selectedInstance)
    {
        auto desktopPath = FS::getDesktopDir();
        if (desktopPath.isEmpty()) {
            // TODO come up with an alternative solution (open "save file" dialog)
            QMessageBox::critical(this, tr("Create instance shortcut"), tr("Couldn't find desktop?!"));
            return;
        }

#if defined(Q_OS_MACOS)
        QString appPath = QApplication::applicationFilePath();
        if (appPath.startsWith("/private/var/")) {
            QMessageBox::critical(this, tr("Create instance shortcut"), tr("The launcher is in the folder it was extracted from, therefore it cannot create shortcuts."));
            return;
        }

        if (FS::createShortcut(FS::PathCombine(desktopPath, m_selectedInstance->name()),
                           appPath, { "--launch", m_selectedInstance->id() },
                           m_selectedInstance->name(), "")) {
            QMessageBox::information(this, tr("Create instance shortcut"), tr("Created a shortcut to this instance on your desktop!"));
        }
        else
        {
            QMessageBox::critical(this, tr("Create instance shortcut"), tr("Failed to create instance shortcut!"));
        }
#elif defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
        QString appPath = QApplication::applicationFilePath();
        if (appPath.startsWith("/tmp/.mount_")) {
            // AppImage!
            appPath = QProcessEnvironment::systemEnvironment().value(QStringLiteral("APPIMAGE"));
            if (appPath.isEmpty())
            {
                QMessageBox::critical(this, tr("Create instance shortcut"), tr("Launcher is running as misconfigured AppImage? ($APPIMAGE environment variable is missing)"));
            }
            else if (appPath.endsWith("/"))
            {
                appPath.chop(1);
            }
        }

        auto icon = APPLICATION->icons()->icon(m_selectedInstance->iconKey());
        if (icon == nullptr)
        {
            icon = APPLICATION->icons()->icon("grass");
        }

        QString iconPath = FS::PathCombine(m_selectedInstance->instanceRoot(), "icon.png");
        
        QFile iconFile(iconPath);
        if (!iconFile.open(QFile::WriteOnly))
        {
            QMessageBox::critical(this, tr("Create instance shortcut"), tr("Failed to create icon for shortcut."));
            return;
        }
        bool success = icon->icon().pixmap(64, 64).save(&iconFile, "PNG");
        iconFile.close();
        
        if (!success)
        {
            iconFile.remove();
            QMessageBox::critical(this, tr("Create instance shortcut"), tr("Failed to create icon for shortcut."));
            return;
        }

        QString desktopFilePath = FS::PathCombine(desktopPath, m_selectedInstance->name() + ".desktop");
        QStringList args;
        if (DesktopServices::isFlatpak()) {
            QFileDialog fileDialog;
            // workaround to make sure the portal file dialog opens in the desktop directory
            fileDialog.setDirectoryUrl(desktopPath);
            desktopFilePath = fileDialog.getSaveFileName(
                            this, tr("Create Shortcut"), desktopFilePath,
                            tr("Desktop Entries (*.desktop)"));
            if (desktopFilePath.isEmpty())
                return; // file dialog canceled by user
            appPath = "flatpak";
            QString flatpakAppId = BuildConfig.LAUNCHER_DESKTOPFILENAME;
            flatpakAppId.remove(".desktop");
            args.append({ "run", flatpakAppId });
        }
        args.append({ "--launch", m_selectedInstance->id() });
        if (FS::createShortcut(desktopFilePath, appPath, args, m_selectedInstance->name(), iconPath)) {
            QMessageBox::information(this, tr("Create instance shortcut"), tr("Created a shortcut to this instance on your desktop!"));
        }
        else
        {
            iconFile.remove();
            QMessageBox::critical(this, tr("Create instance shortcut"), tr("Failed to create instance shortcut!"));
        }
#elif defined(Q_OS_WIN)
        auto icon = APPLICATION->icons()->icon(m_selectedInstance->iconKey());
        if (icon == nullptr)
        {
            icon = APPLICATION->icons()->icon("grass");
        }

        QString iconPath = FS::PathCombine(m_selectedInstance->instanceRoot(), "icon.ico");
        
        // part of fix for weird bug involving the window icon being replaced
        // dunno why it happens, but this 2-line fix seems to be enough, so w/e
        auto appIcon = APPLICATION->getThemedIcon("logo");

        QFile iconFile(iconPath);
        if (!iconFile.open(QFile::WriteOnly))
        {
            QMessageBox::critical(this, tr("Create instance shortcut"), tr("Failed to create icon for shortcut."));
            return;
        }
        bool success = icon->icon().pixmap(64, 64).save(&iconFile, "ICO");
        iconFile.close();

        // restore original window icon
        QGuiApplication::setWindowIcon(appIcon);

        if (!success)
        {
            iconFile.remove();
            QMessageBox::critical(this, tr("Create instance shortcut"), tr("Failed to create icon for shortcut."));
            return;
        }
        
        if (FS::createShortcut(FS::PathCombine(desktopPath, m_selectedInstance->name()),
                           QApplication::applicationFilePath(), { "--launch", m_selectedInstance->id() },
                           m_selectedInstance->name(), iconPath)) {
            QMessageBox::information(this, tr("Create instance shortcut"), tr("Created a shortcut to this instance on your desktop!"));
        }
        else
        {
            iconFile.remove();
            QMessageBox::critical(this, tr("Create instance shortcut"), tr("Failed to create instance shortcut!"));
        }
#else
        QMessageBox::critical(this, tr("Create instance shortcut"), tr("Not supported on your platform!"));
#endif
    }
}

void MainWindow::taskEnd()
{
    QObject *sender = QObject::sender();
    if (sender == m_versionLoadTask)
        m_versionLoadTask = NULL;

    sender->deleteLater();
}

void MainWindow::startTask(Task *task)
{
    connect(task, SIGNAL(succeeded()), SLOT(taskEnd()));
    connect(task, SIGNAL(failed(QString)), SLOT(taskEnd()));
    task->start();
}

void MainWindow::instanceChanged(const QModelIndex &current, const QModelIndex &previous)
{
    if (!current.isValid())
    {
        APPLICATION->settings()->set("SelectedInstance", QString());
        selectionBad();
        return;
    }
    if (m_selectedInstance) {
        disconnect(m_selectedInstance.get(), &BaseInstance::runningStatusChanged, this, &MainWindow::refreshCurrentInstance);
    }
    QString id = current.data(InstanceList::InstanceIDRole).toString();
    m_selectedInstance = APPLICATION->instances()->getInstanceById(id);
    if (m_selectedInstance)
    {
        ui->instanceToolBar->setEnabled(true);
        ui->setInstanceActionsEnabled(true);
        ui->actionLaunchInstance->setEnabled(m_selectedInstance->canLaunch());
        ui->actionLaunchInstanceOffline->setEnabled(m_selectedInstance->canLaunch());
        ui->actionLaunchInstanceDemo->setEnabled(m_selectedInstance->canLaunch());

        // Disable demo-mode if not available.
        auto instance = dynamic_cast<MinecraftInstance*>(m_selectedInstance.get());
        if (instance) {
             ui->actionLaunchInstanceDemo->setEnabled(instance->supportsDemo());
        }

        ui->actionKillInstance->setEnabled(m_selectedInstance->isRunning());
        ui->actionExportInstance->setEnabled(m_selectedInstance->canExport());
        ui->renameButton->setText(m_selectedInstance->name());
        m_statusLeft->setText(m_selectedInstance->getStatusbarDescription());
        updateStatusCenter();
        updateInstanceToolIcon(m_selectedInstance->iconKey());

        updateToolsMenu();

        APPLICATION->settings()->set("SelectedInstance", m_selectedInstance->id());

        connect(m_selectedInstance.get(), &BaseInstance::runningStatusChanged, this, &MainWindow::refreshCurrentInstance);
    }
    else
    {
        ui->instanceToolBar->setEnabled(false);
        ui->setInstanceActionsEnabled(false);
        ui->actionLaunchInstance->setEnabled(false);
        ui->actionLaunchInstanceOffline->setEnabled(false);
        ui->actionLaunchInstanceDemo->setEnabled(false);
        ui->actionKillInstance->setEnabled(false);
        APPLICATION->settings()->set("SelectedInstance", QString());
        selectionBad();
        return;
    }
}

void MainWindow::instanceSelectRequest(QString id)
{
    setSelectedInstanceById(id);
}

void MainWindow::instanceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    auto current = view->selectionModel()->currentIndex();
    QItemSelection test(topLeft, bottomRight);
    if (test.contains(current))
    {
        instanceChanged(current, current);
    }
}

void MainWindow::selectionBad()
{
    // start by reseting everything...
    m_selectedInstance = nullptr;

    statusBar()->clearMessage();
    ui->instanceToolBar->setEnabled(false);
    ui->setInstanceActionsEnabled(false);
    updateToolsMenu();
    ui->renameButton->setText(tr("Rename Instance"));
    updateInstanceToolIcon("grass");

    // ...and then see if we can enable the previously selected instance
    setSelectedInstanceById(APPLICATION->settings()->get("SelectedInstance").toString());
}

void MainWindow::checkInstancePathForProblems()
{
    QString instanceFolder = APPLICATION->settings()->get("InstanceDir").toString();
    if (FS::checkProblemticPathJava(QDir(instanceFolder)))
    {
        QMessageBox warning(this);
        warning.setText(tr("Your instance folder contains \'!\' and this is known to cause Java problems!"));
        warning.setInformativeText(
            tr(
                "You have now two options: <br/>"
                " - change the instance folder in the settings <br/>"
                " - move this installation of %1 to a different folder"
            ).arg(BuildConfig.LAUNCHER_DISPLAYNAME)
        );
        warning.setDefaultButton(QMessageBox::Ok);
        warning.exec();
    }
    auto tempFolderText = tr("This is a problem: <br/>"
                             " - The launcher will likely be deleted without warning by the operating system <br/>"
                             " - close the launcher now and extract it to a real location, not a temporary folder");
    QString pathfoldername = QDir(instanceFolder).absolutePath();
    if (pathfoldername.contains("Rar$", Qt::CaseInsensitive))
    {
        QMessageBox warning(this);
        warning.setText(tr("Your instance folder contains \'Rar$\' - that means you haven't extracted the launcher archive!"));
        warning.setInformativeText(tempFolderText);
        warning.setDefaultButton(QMessageBox::Ok);
        warning.exec();
    }
    else if (pathfoldername.startsWith(QDir::tempPath()) || pathfoldername.contains("/TempState/"))
    {
        QMessageBox warning(this);
        warning.setText(tr("Your instance folder is in a temporary folder: \'%1\'!").arg(QDir::tempPath()));
        warning.setInformativeText(tempFolderText);
        warning.setDefaultButton(QMessageBox::Ok);
        warning.exec();
    }
}

void MainWindow::updateStatusCenter()
{
    m_statusCenter->setVisible(APPLICATION->settings()->get("ShowGlobalGameTime").toBool());

    int timePlayed = APPLICATION->instances()->getTotalPlayTime();
    if (timePlayed > 0) {
        m_statusCenter->setText(tr("Total playtime: %1").arg(Time::prettifyDuration(timePlayed)));
    }
}

void MainWindow::refreshCurrentInstance(bool running)
{
    auto current = view->selectionModel()->currentIndex();
    instanceChanged(current, current);
}
