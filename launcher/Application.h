// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2022 Tayou <tayou@gmx.net>
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

#include <QApplication>
#include <memory>
#include <QDebug>
#include <QFlag>
#include <QIcon>
#include <QDateTime>
#include <QUrl>
#include <updater/GoUpdate.h>

#include <BaseInstance.h>

#include "minecraft/launch/MinecraftServerTarget.h"

class LaunchController;
class LocalPeer;
class InstanceWindow;
class MainWindow;
class SetupWizard;
class GenericPageProvider;
class QFile;
class HttpMetaCache;
class SettingsObject;
class InstanceList;
class AccountList;
class IconList;
class QNetworkAccessManager;
class JavaInstallList;
class UpdateChecker;
class BaseProfilerFactory;
class BaseDetachedToolFactory;
class TranslationsModel;
class ITheme;
class MCEditTool;
class ThemeManager;

namespace Meta {
    class Index;
}

#if defined(APPLICATION)
#undef APPLICATION
#endif
#define APPLICATION (static_cast<Application *>(QCoreApplication::instance()))

class Application : public QApplication
{
    // friends for the purpose of limiting access to deprecated stuff
    Q_OBJECT
public:
    enum Status {
        StartingUp,
        Failed,
        Succeeded,
        Initialized
    };

    enum Capability {
        None = 0,

        SupportsMSA = 1 << 0,
        SupportsFlame = 1 << 1,
        SupportsGameMode = 1 << 2,
        SupportsMangoHud = 1 << 3,
    };
    Q_DECLARE_FLAGS(Capabilities, Capability)

public:
    Application(int &argc, char **argv);
    virtual ~Application();

    bool event(QEvent* event) override;

    std::shared_ptr<SettingsObject> settings() const {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings;
    }

    qint64 timeSinceStart() const {
        return startTime.msecsTo(QDateTime::currentDateTime());
    }

    QIcon getThemedIcon(const QString& name);

    void setIconTheme(const QString& name);

    QList<ITheme*> getValidApplicationThemes();

    void setApplicationTheme(const QString& name, bool initial);

    shared_qobject_ptr<UpdateChecker> updateChecker() {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateChecker;
    }

    std::shared_ptr<TranslationsModel> translations();

    std::shared_ptr<JavaInstallList> javalist();

    std::shared_ptr<InstanceList> instances() const {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instances;
    }

    std::shared_ptr<IconList> icons() const {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_icons;
    }

    MCEditTool *mcedit() const {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mcedit.get();
    }

    shared_qobject_ptr<AccountList> accounts() const {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_accounts;
    }

    Status status() const {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_status;
    }

    const QMap<QString, std::shared_ptr<BaseProfilerFactory>> &profilers() const {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_profilers;
    }

    void updateProxySettings(QString proxyTypeStr, QString addr, int port, QString user, QString password);

    shared_qobject_ptr<QNetworkAccessManager> network();

    shared_qobject_ptr<HttpMetaCache> metacache();

    shared_qobject_ptr<Meta::Index> metadataIndex();

    void updateCapabilities();

    /*!
     * Finds and returns the full path to a jar file.
     * Returns a null-string if it could not be found.
     */
    QString getJarPath(QString jarFile);

    QString getMSAClientID();
    QString getFlameAPIKey();
    QString getUserAgent();
    QString getUserAgentUncached();

    /// this is the root of the 'installation'. Used for automatic updates
    const QString &root() {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_rootPath;
    }

    const Capabilities capabilities() {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_capabilities;
    }

    /*!
     * Opens a json file using either a system default editor, or, if not empty, the editor
     * specified in the settings
     */
    bool openJsonEditor(const QString &filename);

    InstanceWindow *showInstanceWindow(InstancePtr instance, QString page = QString());
    MainWindow *showMainWindow(bool minimized = false);

    void updateIsRunning(bool running);
    bool updatesAreAllowed();

    void ShowGlobalSettings(class QWidget * parent, QString open_page = QString());

    int suitableMaxMem();

signals:
    void updateAllowedChanged(bool status);
    void globalSettingsAboutToOpen();
    void globalSettingsClosed();

#ifdef Q_OS_MACOS
    void clickedOnDock();
#endif

public slots:
    bool launch(
        InstancePtr instance,
        bool online = true,
        bool demo = false,
        BaseProfilerFactory *profiler = nullptr,
        MinecraftServerTargetPtr serverToJoin = nullptr,
        MinecraftAccountPtr accountToUse = nullptr
    );
    bool kill(InstancePtr instance);
    void closeCurrentWindow();

private slots:
    void on_windowClose();
    void messageReceived(const QByteArray & message);
    void controllerSucceeded();
    void controllerFailed(const QString & error);
    void setupWizardFinished(int status);

private:
    bool handleDataMigration(const QString & currentData, const QString & oldData, const QString & name, const QString & configFile) const;
    bool createSetupWizard();
    void performMainStartupAction();

    // sets the fatal error message and hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_status to Failed.
    void showFatalErrorMessage(const QString & title, const QString & content);

private:
    void addRunningInstance();
    void subRunningInstance();
    bool shouldExitNow() const;

private:
    QDateTime startTime;

    shared_qobject_ptr<QNetworkAccessManager> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network;

    shared_qobject_ptr<UpdateChecker> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateChecker;
    shared_qobject_ptr<AccountList> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_accounts;

    shared_qobject_ptr<HttpMetaCache> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metacache;
    shared_qobject_ptr<Meta::Index> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metadataIndex;

    std::shared_ptr<SettingsObject> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_settings;
    std::shared_ptr<InstanceList> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instances;
    std::shared_ptr<IconList> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_icons;
    std::shared_ptr<JavaInstallList> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_javalist;
    std::shared_ptr<TranslationsModel> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_translations;
    std::shared_ptr<GenericPageProvider> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_globalSettingsProvider;
    std::unique_ptr<MCEditTool> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mcedit;
    QSet<QString> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_features;
    std::unique_ptr<ThemeManager> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_themeManager;

    QMap<QString, std::shared_ptr<BaseProfilerFactory>> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_profilers;

    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_rootPath;
    Status hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_status = Application::StartingUp;
    Capabilities hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_capabilities;

#ifdef Q_OS_MACOS
    Qt::ApplicationState hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_prevAppState = Qt::ApplicationInactive;
#endif

#if defined Q_OS_WIN32
    // used on Windows to attach the standard IO streams
    bool consoleAttached = false;
#endif

    // FIXME: attach to instances instead.
    struct InstanceXtras {
        InstanceWindow * window = nullptr;
        shared_qobject_ptr<LaunchController> controller;
    };
    std::map<QString, InstanceXtras> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instanceExtras;

    // main state variables
    size_t hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_openWindows = 0;
    size_t hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_runningInstances = 0;
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_updateRunning = false;

    // main window, if any
    MainWindow * hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mainWindow = nullptr;

    // peer launcher instance connector - used to implement single instance launcher and signalling
    LocalPeer * hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_peerInstance = nullptr;

    SetupWizard * hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_setupWizard = nullptr;
public:
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instanceIdToLaunch;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_serverToJoin;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_profileToUse;
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_liveCheck = false;
    QUrl hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_zipToImport;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instanceIdToShowWindowOf;
    std::unique_ptr<QFile> logFile;
};

