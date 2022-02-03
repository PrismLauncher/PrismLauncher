/* Copyright 2013-2021 MultiMC Contributors
 *
 * Authors: Andrew Okin
 *          Peterix
 *          Orochimarufan <orochimarufan.x3@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "Application.h"
#include "BuildConfig.h"

#include "MainWindow.h"

#include <QtCore/QVariant>
#include <QtCore/QUrl>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include <QtGui/QKeyEvent>

#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidgetAction>
#include <QtWidgets/QProgressDialog>
#include <QtWidgets/QShortcut>

#include <BaseInstance.h>
#include <InstanceList.h>
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
#include <notifications/NotificationChecker.h>
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
#include "ui/dialogs/ProgressDialog.h"
#include "ui/dialogs/AboutDialog.h"
#include "ui/dialogs/VersionSelectDialog.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/IconPickerDialog.h"
#include "ui/dialogs/CopyInstanceDialog.h"
#include "ui/dialogs/UpdateDialog.h"
#include "ui/dialogs/EditAccountDialog.h"
#include "ui/dialogs/NotificationDialog.h"
#include "ui/dialogs/ExportInstanceDialog.h"

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
                result = result.arg(BuildConfig.LAUNCHER_NAME);
            }
            m_contained->setText(result);
        }
        if(m_tooltip)
        {
            QString result;
            result = QApplication::translate("MainWindow", m_tooltip);
            if(result.contains("%1")) {
                result = result.arg(BuildConfig.LAUNCHER_NAME);
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
    TranslatedAction actionPatreon;
    TranslatedAction actionMoreNews;
    TranslatedAction actionManageAccounts;
    TranslatedAction actionLaunchInstance;
    TranslatedAction actionRenameInstance;
    TranslatedAction actionChangeInstGroup;
    TranslatedAction actionChangeInstIcon;
    TranslatedAction actionEditInstNotes;
    TranslatedAction actionEditInstance;
    TranslatedAction actionWorlds;
    TranslatedAction actionMods;
    TranslatedAction actionViewSelectedInstFolder;
    TranslatedAction actionViewSelectedMCFolder;
    TranslatedAction actionViewSelectedModsFolder;
    TranslatedAction actionDeleteInstance;
    TranslatedAction actionConfig_Folder;
    TranslatedAction actionCAT;
    TranslatedAction actionCopyInstance;
    TranslatedAction actionLaunchInstanceOffline;
    TranslatedAction actionScreenshots;
    TranslatedAction actionExportInstance;
    QVector<TranslatedAction *> all_actions;

    LabeledToolButton *renameButton = nullptr;
    LabeledToolButton *changeIconButton = nullptr;

    QMenu * foldersMenu = nullptr;
    TranslatedToolButton foldersMenuButton;
    TranslatedAction actionViewInstanceFolder;
    TranslatedAction actionViewCentralModsFolder;

    QMenu * helpMenu = nullptr;
    TranslatedToolButton helpMenuButton;
    TranslatedAction actionReportBug;
    TranslatedAction actionDISCORD;
    TranslatedAction actionREDDIT;
    TranslatedAction actionAbout;

    QVector<TranslatedToolButton *> all_toolbuttons;

    QWidget *centralWidget = nullptr;
    QHBoxLayout *horizontalLayout = nullptr;
    QStatusBar *statusBar = nullptr;

    TranslatedToolbar mainToolBar;
    TranslatedToolbar instanceToolBar;
    TranslatedToolbar newsToolBar;
    QVector<TranslatedToolbar *> all_toolbars;
    bool m_kill = false;

    void updateLaunchAction()
    {
        if(m_kill)
        {
            actionLaunchInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Kill"));
            actionLaunchInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Kill the running instance"));
        }
        else
        {
            actionLaunchInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Launch"));
            actionLaunchInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Launch the selected instance."));
        }
        actionLaunchInstance.retranslate();
    }
    void setLaunchAction(bool kill)
    {
        m_kill = kill;
        updateLaunchAction();
    }

    void createMainToolbar(QMainWindow *MainWindow)
    {
        mainToolBar = TranslatedToolbar(MainWindow);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        mainToolBar->setMovable(false);
        mainToolBar->setAllowedAreas(Qt::TopToolBarArea);
        mainToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        mainToolBar->setFloatable(false);
        mainToolBar.setWindowTitleId(QT_TRANSLATE_NOOP("MainWindow", "Main Toolbar"));

        actionAddInstance = TranslatedAction(MainWindow);
        actionAddInstance->setObjectName(QStringLiteral("actionAddInstance"));
        actionAddInstance->setIcon(APPLICATION->getThemedIcon("new"));
        actionAddInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Add Instance"));
        actionAddInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Add a new instance."));
        all_actions.append(&actionAddInstance);
        mainToolBar->addAction(actionAddInstance);

        mainToolBar->addSeparator();

        foldersMenu = new QMenu(MainWindow);
        foldersMenu->setToolTipsVisible(true);

        actionViewInstanceFolder = TranslatedAction(MainWindow);
        actionViewInstanceFolder->setObjectName(QStringLiteral("actionViewInstanceFolder"));
        actionViewInstanceFolder->setIcon(APPLICATION->getThemedIcon("viewfolder"));
        actionViewInstanceFolder.setTextId(QT_TRANSLATE_NOOP("MainWindow", "View Instance Folder"));
        actionViewInstanceFolder.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the instance folder in a file browser."));
        all_actions.append(&actionViewInstanceFolder);
        foldersMenu->addAction(actionViewInstanceFolder);

        actionViewCentralModsFolder = TranslatedAction(MainWindow);
        actionViewCentralModsFolder->setObjectName(QStringLiteral("actionViewCentralModsFolder"));
        actionViewCentralModsFolder->setIcon(APPLICATION->getThemedIcon("centralmods"));
        actionViewCentralModsFolder.setTextId(QT_TRANSLATE_NOOP("MainWindow", "View Central Mods Folder"));
        actionViewCentralModsFolder.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the central mods folder in a file browser."));
        all_actions.append(&actionViewCentralModsFolder);
        foldersMenu->addAction(actionViewCentralModsFolder);

        foldersMenuButton = TranslatedToolButton(MainWindow);
        foldersMenuButton.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Folders"));
        foldersMenuButton.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open one of the folders shared between instances."));
        foldersMenuButton->setMenu(foldersMenu);
        foldersMenuButton->setPopupMode(QToolButton::InstantPopup);
        foldersMenuButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        foldersMenuButton->setIcon(APPLICATION->getThemedIcon("viewfolder"));
        foldersMenuButton->setFocusPolicy(Qt::NoFocus);
        all_toolbuttons.append(&foldersMenuButton);
        QWidgetAction* foldersButtonAction = new QWidgetAction(MainWindow);
        foldersButtonAction->setDefaultWidget(foldersMenuButton);
        mainToolBar->addAction(foldersButtonAction);

        actionSettings = TranslatedAction(MainWindow);
        actionSettings->setObjectName(QStringLiteral("actionSettings"));
        actionSettings->setIcon(APPLICATION->getThemedIcon("settings"));
        actionSettings->setMenuRole(QAction::PreferencesRole);
        actionSettings.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Settings"));
        actionSettings.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Change settings."));
        all_actions.append(&actionSettings);
        mainToolBar->addAction(actionSettings);

        helpMenu = new QMenu(MainWindow);
        helpMenu->setToolTipsVisible(true);

        if (!BuildConfig.BUG_TRACKER_URL.isEmpty()) {
            actionReportBug = TranslatedAction(MainWindow);
            actionReportBug->setObjectName(QStringLiteral("actionReportBug"));
            actionReportBug->setIcon(APPLICATION->getThemedIcon("bug"));
            actionReportBug.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Report a Bug"));
            actionReportBug.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the bug tracker to report a bug with %1."));
            all_actions.append(&actionReportBug);
            helpMenu->addAction(actionReportBug);
        }

        if (!BuildConfig.DISCORD_URL.isEmpty()) {
            actionDISCORD = TranslatedAction(MainWindow);
            actionDISCORD->setObjectName(QStringLiteral("actionDISCORD"));
            actionDISCORD->setIcon(APPLICATION->getThemedIcon("discord"));
            actionDISCORD.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Discord"));
            actionDISCORD.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open %1 discord voice chat."));
            all_actions.append(&actionDISCORD);
            helpMenu->addAction(actionDISCORD);
        }

        if (!BuildConfig.SUBREDDIT_URL.isEmpty()) {
            actionREDDIT = TranslatedAction(MainWindow);
            actionREDDIT->setObjectName(QStringLiteral("actionREDDIT"));
            actionREDDIT->setIcon(APPLICATION->getThemedIcon("reddit-alien"));
            actionREDDIT.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Reddit"));
            actionREDDIT.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open %1 subreddit."));
            all_actions.append(&actionREDDIT);
            helpMenu->addAction(actionREDDIT);
        }

        actionAbout = TranslatedAction(MainWindow);
        actionAbout->setObjectName(QStringLiteral("actionAbout"));
        actionAbout->setIcon(APPLICATION->getThemedIcon("about"));
        actionAbout->setMenuRole(QAction::AboutRole);
        actionAbout.setTextId(QT_TRANSLATE_NOOP("MainWindow", "About %1"));
        actionAbout.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "View information about %1."));
        all_actions.append(&actionAbout);
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
            actionCheckUpdate = TranslatedAction(MainWindow);
            actionCheckUpdate->setObjectName(QStringLiteral("actionCheckUpdate"));
            actionCheckUpdate->setIcon(APPLICATION->getThemedIcon("checkupdate"));
            actionCheckUpdate.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Update"));
            actionCheckUpdate.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Check for new updates for %1."));
            all_actions.append(&actionCheckUpdate);
            mainToolBar->addAction(actionCheckUpdate);
        }

        mainToolBar->addSeparator();

        actionPatreon = TranslatedAction(MainWindow);
        actionPatreon->setObjectName(QStringLiteral("actionPatreon"));
        actionPatreon->setIcon(APPLICATION->getThemedIcon("patreon"));
        actionPatreon.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Support %1"));
        actionPatreon.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the %1 Patreon page."));
        all_actions.append(&actionPatreon);
        mainToolBar->addAction(actionPatreon);

        actionCAT = TranslatedAction(MainWindow);
        actionCAT->setObjectName(QStringLiteral("actionCAT"));
        actionCAT->setCheckable(true);
        actionCAT->setIcon(APPLICATION->getThemedIcon("cat"));
        actionCAT.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Meow"));
        actionCAT.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "It's a fluffy kitty :3"));
        actionCAT->setPriority(QAction::LowPriority);
        all_actions.append(&actionCAT);
        mainToolBar->addAction(actionCAT);

        // profile menu and its actions
        actionManageAccounts = TranslatedAction(MainWindow);
        actionManageAccounts->setObjectName(QStringLiteral("actionManageAccounts"));
        actionManageAccounts.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Manage Accounts"));
        // FIXME: no tooltip!
        actionManageAccounts->setCheckable(false);
        actionManageAccounts->setIcon(APPLICATION->getThemedIcon("accounts"));
        all_actions.append(&actionManageAccounts);

        all_toolbars.append(&mainToolBar);
        MainWindow->addToolBar(Qt::TopToolBarArea, mainToolBar);
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
        newsToolBar->setMovable(false);
        newsToolBar->setAllowedAreas(Qt::BottomToolBarArea);
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

    void createInstanceToolbar(QMainWindow *MainWindow)
    {
        instanceToolBar = TranslatedToolbar(MainWindow);
        instanceToolBar->setObjectName(QStringLiteral("instanceToolBar"));
        // disabled until we have an instance selected
        instanceToolBar->setEnabled(false);
        instanceToolBar->setAllowedAreas(Qt::LeftToolBarArea | Qt::RightToolBarArea);
        instanceToolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
        instanceToolBar->setFloatable(false);
        instanceToolBar->setWindowTitle(QT_TRANSLATE_NOOP("MainWindow", "Instance Toolbar"));

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
        instanceToolBar->addWidget(changeIconButton);

        // NOTE: not added to toolbar, but used for instance context menu (right click)
        actionRenameInstance = TranslatedAction(MainWindow);
        actionRenameInstance->setObjectName(QStringLiteral("actionRenameInstance"));
        actionRenameInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Rename"));
        actionRenameInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Rename the selected instance."));
        all_actions.append(&actionRenameInstance);

        // the rename label is inside the rename tool button
        renameButton = new LabeledToolButton(MainWindow);
        renameButton->setObjectName(QStringLiteral("renameButton"));
        renameButton->setToolTip(actionRenameInstance->toolTip());
        renameButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        instanceToolBar->addWidget(renameButton);

        instanceToolBar->addSeparator();

        actionLaunchInstance = TranslatedAction(MainWindow);
        actionLaunchInstance->setObjectName(QStringLiteral("actionLaunchInstance"));
        all_actions.append(&actionLaunchInstance);
        instanceToolBar->addAction(actionLaunchInstance);

        actionLaunchInstanceOffline = TranslatedAction(MainWindow);
        actionLaunchInstanceOffline->setObjectName(QStringLiteral("actionLaunchInstanceOffline"));
        actionLaunchInstanceOffline.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Launch Offline"));
        actionLaunchInstanceOffline.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Launch the selected instance in offline mode."));
        all_actions.append(&actionLaunchInstanceOffline);
        instanceToolBar->addAction(actionLaunchInstanceOffline);

        instanceToolBar->addSeparator();

        actionEditInstance = TranslatedAction(MainWindow);
        actionEditInstance->setObjectName(QStringLiteral("actionEditInstance"));
        actionEditInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Edit Instance"));
        actionEditInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Change the instance settings, mods and versions."));
        all_actions.append(&actionEditInstance);
        instanceToolBar->addAction(actionEditInstance);

        actionEditInstNotes = TranslatedAction(MainWindow);
        actionEditInstNotes->setObjectName(QStringLiteral("actionEditInstNotes"));
        actionEditInstNotes.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Edit Notes"));
        actionEditInstNotes.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Edit the notes for the selected instance."));
        all_actions.append(&actionEditInstNotes);
        instanceToolBar->addAction(actionEditInstNotes);

        actionMods = TranslatedAction(MainWindow);
        actionMods->setObjectName(QStringLiteral("actionMods"));
        actionMods.setTextId(QT_TRANSLATE_NOOP("MainWindow", "View Mods"));
        actionMods.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "View the mods of this instance."));
        all_actions.append(&actionMods);
        instanceToolBar->addAction(actionMods);

        actionWorlds = TranslatedAction(MainWindow);
        actionWorlds->setObjectName(QStringLiteral("actionWorlds"));
        actionWorlds.setTextId(QT_TRANSLATE_NOOP("MainWindow", "View Worlds"));
        actionWorlds.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "View the worlds of this instance."));
        all_actions.append(&actionWorlds);
        instanceToolBar->addAction(actionWorlds);

        actionScreenshots = TranslatedAction(MainWindow);
        actionScreenshots->setObjectName(QStringLiteral("actionScreenshots"));
        actionScreenshots.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Manage Screenshots"));
        actionScreenshots.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "View and upload screenshots for this instance."));
        all_actions.append(&actionScreenshots);
        instanceToolBar->addAction(actionScreenshots);

        actionChangeInstGroup = TranslatedAction(MainWindow);
        actionChangeInstGroup->setObjectName(QStringLiteral("actionChangeInstGroup"));
        actionChangeInstGroup.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Change Group"));
        actionChangeInstGroup.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Change the selected instance's group."));
        all_actions.append(&actionChangeInstGroup);
        instanceToolBar->addAction(actionChangeInstGroup);

        instanceToolBar->addSeparator();

        actionViewSelectedMCFolder = TranslatedAction(MainWindow);
        actionViewSelectedMCFolder->setObjectName(QStringLiteral("actionViewSelectedMCFolder"));
        actionViewSelectedMCFolder.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Minecraft Folder"));
        actionViewSelectedMCFolder.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the selected instance's minecraft folder in a file browser."));
        all_actions.append(&actionViewSelectedMCFolder);
        instanceToolBar->addAction(actionViewSelectedMCFolder);

        /*
        actionViewSelectedModsFolder = TranslatedAction(MainWindow);
        actionViewSelectedModsFolder->setObjectName(QStringLiteral("actionViewSelectedModsFolder"));
        actionViewSelectedModsFolder.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Mods Folder"));
        actionViewSelectedModsFolder.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the selected instance's mods folder in a file browser."));
        all_actions.append(&actionViewSelectedModsFolder);
        instanceToolBar->addAction(actionViewSelectedModsFolder);
        */

        actionConfig_Folder = TranslatedAction(MainWindow);
        actionConfig_Folder->setObjectName(QStringLiteral("actionConfig_Folder"));
        actionConfig_Folder.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Config Folder"));
        actionConfig_Folder.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the instance's config folder."));
        all_actions.append(&actionConfig_Folder);
        instanceToolBar->addAction(actionConfig_Folder);

        actionViewSelectedInstFolder = TranslatedAction(MainWindow);
        actionViewSelectedInstFolder->setObjectName(QStringLiteral("actionViewSelectedInstFolder"));
        actionViewSelectedInstFolder.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Instance Folder"));
        actionViewSelectedInstFolder.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Open the selected instance's root folder in a file browser."));
        all_actions.append(&actionViewSelectedInstFolder);
        instanceToolBar->addAction(actionViewSelectedInstFolder);

        instanceToolBar->addSeparator();

        actionExportInstance = TranslatedAction(MainWindow);
        actionExportInstance->setObjectName(QStringLiteral("actionExportInstance"));
        actionExportInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Export Instance"));
        actionExportInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Export the selected instance as a zip file."));
        all_actions.append(&actionExportInstance);
        instanceToolBar->addAction(actionExportInstance);

        actionDeleteInstance = TranslatedAction(MainWindow);
        actionDeleteInstance->setObjectName(QStringLiteral("actionDeleteInstance"));
        actionDeleteInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Delete"));
        actionDeleteInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Delete the selected instance."));
        all_actions.append(&actionDeleteInstance);
        instanceToolBar->addAction(actionDeleteInstance);

        actionCopyInstance = TranslatedAction(MainWindow);
        actionCopyInstance->setObjectName(QStringLiteral("actionCopyInstance"));
        actionCopyInstance->setIcon(APPLICATION->getThemedIcon("copy"));
        actionCopyInstance.setTextId(QT_TRANSLATE_NOOP("MainWindow", "Copy Instance"));
        actionCopyInstance.setTooltipId(QT_TRANSLATE_NOOP("MainWindow", "Copy the selected instance."));
        all_actions.append(&actionCopyInstance);
        instanceToolBar->addAction(actionCopyInstance);

        all_toolbars.append(&instanceToolBar);
        MainWindow->addToolBar(Qt::RightToolBarArea, instanceToolBar);
    }

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
        {
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        }
        MainWindow->resize(800, 600);
        MainWindow->setWindowIcon(APPLICATION->getThemedIcon("logo"));
        MainWindow->setWindowTitle(BuildConfig.LAUNCHER_DISPLAYNAME);
#ifndef QT_NO_ACCESSIBILITY
        MainWindow->setAccessibleName(BuildConfig.LAUNCHER_NAME);
#endif

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

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        QString winTitle = tr("%1 - Version %2", "Launcher - Version X").arg(BuildConfig.LAUNCHER_DISPLAYNAME, BuildConfig.printableVersionString());
        if (!BuildConfig.BUILD_PLATFORM.isEmpty())
        {
            winTitle += tr(" on %1", "on platform, as in operating system").arg(BuildConfig.BUILD_PLATFORM);
        }
        MainWindow->setWindowTitle(winTitle);
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
    }

    {
        auto checker = new NotificationChecker();
        checker->setNotificationsUrl(QUrl(BuildConfig.NOTIFICATION_URL));
        checker->setApplicationChannel(BuildConfig.VERSION_CHANNEL);
        checker->setApplicationPlatform(BuildConfig.BUILD_PLATFORM);
        checker->setApplicationFullVersion(BuildConfig.FULL_VERSION_STR);
        m_notificationChecker.reset(checker);
        connect(m_notificationChecker.get(), &NotificationChecker::notificationCheckFinished, this, &MainWindow::notificationsChanged);
        checker->checkForNotifications();
    }

    setSelectedInstanceById(APPLICATION->settings()->get("SelectedInstance").toString());

    // removing this looks stupid
    view->setFocus();

    retranslateUi();
}

void MainWindow::retranslateUi()
{
    auto accounts = APPLICATION->accounts();
    MinecraftAccountPtr defaultAccount = accounts->defaultAccount();
    if(defaultAccount) {
        auto profileLabel = profileInUseFilter(defaultAccount->profileName(), defaultAccount->isInUse());
        accountMenuButton->setText(profileLabel);
    }
    else {
        accountMenuButton->setText(tr("Profiles"));
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
    return filteredMenu;
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

        QAction *actionVoid = new QAction(BuildConfig.LAUNCHER_NAME, this);
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

void MainWindow::updateToolsMenu()
{
    QToolButton *launchButton = dynamic_cast<QToolButton*>(ui->instanceToolBar->widgetForAction(ui->actionLaunchInstance));
    QToolButton *launchOfflineButton = dynamic_cast<QToolButton*>(ui->instanceToolBar->widgetForAction(ui->actionLaunchInstanceOffline));

    if(!m_selectedInstance || m_selectedInstance->isRunning())
    {
        ui->actionLaunchInstance->setMenu(nullptr);
        ui->actionLaunchInstanceOffline->setMenu(nullptr);
        launchButton->setPopupMode(QToolButton::InstantPopup);
        launchOfflineButton->setPopupMode(QToolButton::InstantPopup);
        return;
    }

    QMenu *launchMenu = ui->actionLaunchInstance->menu();
    QMenu *launchOfflineMenu = ui->actionLaunchInstanceOffline->menu();
    launchButton->setPopupMode(QToolButton::MenuButtonPopup);
    launchOfflineButton->setPopupMode(QToolButton::MenuButtonPopup);
    if (launchMenu)
    {
        launchMenu->clear();
    }
    else
    {
        launchMenu = new QMenu(this);
    }
    if (launchOfflineMenu) {
        launchOfflineMenu->clear();
    }
    else
    {
        launchOfflineMenu = new QMenu(this);
    }

    QAction *normalLaunch = launchMenu->addAction(tr("Launch"));
    QAction *normalLaunchOffline = launchOfflineMenu->addAction(tr("Launch Offline"));
    connect(normalLaunch, &QAction::triggered, [this]()
            {
                APPLICATION->launch(m_selectedInstance, true);
            });
    connect(normalLaunchOffline, &QAction::triggered, [this]()
            {
                APPLICATION->launch(m_selectedInstance, false);
            });
    QString profilersTitle = tr("Profilers");
    launchMenu->addSeparator()->setText(profilersTitle);
    launchOfflineMenu->addSeparator()->setText(profilersTitle);
    for (auto profiler : APPLICATION->profilers().values())
    {
        QAction *profilerAction = launchMenu->addAction(profiler->name());
        QAction *profilerOfflineAction = launchOfflineMenu->addAction(profiler->name());
        QString error;
        if (!profiler->check(&error))
        {
            profilerAction->setDisabled(true);
            profilerOfflineAction->setDisabled(true);
            QString profilerToolTip = tr("Profiler not setup correctly. Go into settings, \"External Tools\".");
            profilerAction->setToolTip(profilerToolTip);
            profilerOfflineAction->setToolTip(profilerToolTip);
        }
        else
        {
            connect(profilerAction, &QAction::triggered, [this, profiler]()
                    {
                        APPLICATION->launch(m_selectedInstance, true, profiler.get());
                    });
            connect(profilerOfflineAction, &QAction::triggered, [this, profiler]()
                    {
                        APPLICATION->launch(m_selectedInstance, false, profiler.get());
                    });
        }
    }
    ui->actionLaunchInstance->setMenu(launchMenu);
    ui->actionLaunchInstanceOffline->setMenu(launchOfflineMenu);
}

void MainWindow::repopulateAccountsMenu()
{
    accountMenu->clear();

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
        QAction *action = new QAction(tr("No accounts added!"), this);
        action->setEnabled(false);
        accountMenu->addAction(action);
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
            accountMenu->addAction(action);
            connect(action, SIGNAL(triggered(bool)), SLOT(changeActiveAccount()));
        }
    }

    accountMenu->addSeparator();

    QAction *action = new QAction(tr("No Default Account"), this);
    action->setCheckable(true);
    action->setIcon(APPLICATION->getThemedIcon("noaccount"));
    action->setData(-1);
    if (!defaultAccount) {
        action->setChecked(true);
    }

    accountMenu->addAction(action);
    connect(action, SIGNAL(triggered(bool)), SLOT(changeActiveAccount()));

    accountMenu->addSeparator();
    accountMenu->addAction(ui->actionManageAccounts);
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
    accountMenuButton->setText(tr("Profiles"));
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
    }
    else
    {
        QList<NewsEntryPtr> entries = m_newsChecker->getNewsEntries();
        if (entries.length() > 0)
        {
            newsLabel->setText(entries[0]->title);
            newsLabel->setEnabled(true);
        }
        else
        {
            newsLabel->setText(tr("No news available."));
            newsLabel->setEnabled(false);
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
    QStringList split = string.split(',', QString::SkipEmptyParts);
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
void MainWindow::notificationsChanged()
{
    QList<NotificationChecker::NotificationEntry> entries = m_notificationChecker->notificationEntries();
    QList<int> shownNotifications = stringToIntList(APPLICATION->settings()->get("ShownNotifications").toString());
    for (auto it = entries.begin(); it != entries.end(); ++it)
    {
        NotificationChecker::NotificationEntry entry = *it;
        if (!shownNotifications.contains(entry.id))
        {
            NotificationDialog dialog(entry, this);
            if (dialog.exec() == NotificationDialog::DontShowAgain)
            {
                shownNotifications.append(entry.id);
            }
        }
    }
    APPLICATION->settings()->set("ShownNotifications", intListToString(shownNotifications));
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
        QString cat;
        if(non_stupid_abs(now.daysTo(xmas)) <= 4) {
            cat = "catmas";
        }
        else if (non_stupid_abs(now.daysTo(birthday)) <= 12) {
            cat = "cattiversary";
        }
        else {
            cat = "kitteh";
        }
        view->setStyleSheet(QString(R"(
InstanceView
{
    background-image: url(:/backgrounds/%1);
    background-attachment: fixed;
    background-clip: padding;
    background-position: top right;
    background-repeat: none;
    background-color:palette(base);
})").arg(cat));
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

    auto copyTask = new InstanceCopyTask(m_selectedInstance, copyInstDlg.shouldCopySaves(), copyInstDlg.shouldKeepPlaytime());
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
    for(auto & url:urls)
    {
        if(url.isLocalFile())
        {
            addInstance(url.toLocalFile());
        }
        else
        {
            addInstance(url.toString());
        }
        // Only process one dropped file...
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
        auto reply = QMessageBox::question(this, tr("Delete group"), tr("Are you sure you want to delete the group %1")
            .arg(groupName), QMessageBox::Yes | QMessageBox::No);
        if(reply == QMessageBox::Yes)
        {
            APPLICATION->instances()->deleteGroup(groupName);
        }
    }
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

void MainWindow::on_actionConfig_Folder_triggered()
{
    if (m_selectedInstance)
    {
        QString str = m_selectedInstance->instanceConfigFolder();
        DesktopServices::openDirectory(QDir(str).absolutePath());
    }
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
    updateToolsMenu();
    updateStatusCenter();
    update();
}

void MainWindow::on_actionInstanceSettings_triggered()
{
    APPLICATION->showInstanceWindow(m_selectedInstance, "settings");
}

void MainWindow::on_actionEditInstNotes_triggered()
{
    APPLICATION->showInstanceWindow(m_selectedInstance, "notes");
}

void MainWindow::on_actionWorlds_triggered()
{
    APPLICATION->showInstanceWindow(m_selectedInstance, "worlds");
}

void MainWindow::on_actionMods_triggered()
{
    APPLICATION->showInstanceWindow(m_selectedInstance, "mods");
}

void MainWindow::on_actionEditInstance_triggered()
{
    APPLICATION->showInstanceWindow(m_selectedInstance);
}

void MainWindow::on_actionScreenshots_triggered()
{
    APPLICATION->showInstanceWindow(m_selectedInstance, "screenshots");
}

void MainWindow::on_actionManageAccounts_triggered()
{
    APPLICATION->ShowGlobalSettings(this, "accounts");
}

void MainWindow::on_actionReportBug_triggered()
{
    DesktopServices::openUrl(QUrl(BuildConfig.BUG_TRACKER_URL));
}

void MainWindow::on_actionPatreon_triggered()
{
    DesktopServices::openUrl(QUrl("https://www.patreon.com/multimc"));
}

void MainWindow::on_actionMoreNews_triggered()
{
    DesktopServices::openUrl(QUrl("https://multimc.org/posts.html"));
}

void MainWindow::newsButtonClicked()
{
    QList<NewsEntryPtr> entries = m_newsChecker->getNewsEntries();
    if (entries.count() > 0)
    {
        DesktopServices::openUrl(QUrl(entries[0]->link));
    }
    else
    {
        DesktopServices::openUrl(QUrl("https://multimc.org/posts.html"));
    }
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::on_actionDeleteInstance_triggered()
{
    if (!m_selectedInstance)
    {
        return;
    }
    auto id = m_selectedInstance->id();
    auto response = CustomMessageBox::selectable(
        this,
        tr("CAREFUL!"),
        tr("About to delete: %1\nThis is permanent and will completely delete the instance.\n\nAre you sure?").arg(m_selectedInstance->name()),
        QMessageBox::Warning,
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    )->exec();
    if (response == QMessageBox::Yes)
    {
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

void MainWindow::on_actionViewSelectedMCFolder_triggered()
{
    if (m_selectedInstance)
    {
        QString str = m_selectedInstance->gameRoot();
        if (!FS::ensureFilePathExists(str))
        {
            // TODO: report error
            return;
        }
        DesktopServices::openDirectory(QDir(str).absolutePath());
    }
}

void MainWindow::on_actionViewSelectedModsFolder_triggered()
{
    if (m_selectedInstance)
    {
        QString str = m_selectedInstance->modsRoot();
        if (!FS::ensureFilePathExists(str))
        {
            // TODO: report error
            return;
        }
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
    if (!m_selectedInstance)
    {
        return;
    }
    if(m_selectedInstance->isRunning())
    {
        APPLICATION->kill(m_selectedInstance);
    }
    else
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
    QString id = current.data(InstanceList::InstanceIDRole).toString();
    m_selectedInstance = APPLICATION->instances()->getInstanceById(id);
    if (m_selectedInstance)
    {
        ui->instanceToolBar->setEnabled(true);
        if(m_selectedInstance->isRunning())
        {
            ui->actionLaunchInstance->setEnabled(true);
            ui->setLaunchAction(true);
        }
        else
        {
            ui->actionLaunchInstance->setEnabled(m_selectedInstance->canLaunch());
            ui->setLaunchAction(false);
        }
        ui->actionLaunchInstanceOffline->setEnabled(m_selectedInstance->canLaunch());
        ui->actionExportInstance->setEnabled(m_selectedInstance->canExport());
        ui->renameButton->setText(m_selectedInstance->name());
        m_statusLeft->setText(m_selectedInstance->getStatusbarDescription());
        updateStatusCenter();
        updateInstanceToolIcon(m_selectedInstance->iconKey());

        updateToolsMenu();

        APPLICATION->settings()->set("SelectedInstance", m_selectedInstance->id());
    }
    else
    {
        ui->instanceToolBar->setEnabled(false);
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
            ).arg(BuildConfig.LAUNCHER_NAME)
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
