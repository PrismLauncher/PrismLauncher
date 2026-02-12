// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2022 Tayou <git@tayou.org>
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
#include <QDateTime>
#include <QDebug>
#include <QFlag>
#include <QIcon>
#include <QMutex>
#include <QUrl>
#include <memory>

#include <BaseInstance.h>

#include "launch/LogModel.h"
#include "minecraft/launch/MinecraftTarget.h"

class LaunchController;
class LocalPeer;
class InstanceWindow;
class MainWindow;
class ViewLogWindow;
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
class ExternalUpdater;
class BaseProfilerFactory;
class BaseDetachedToolFactory;
class TranslationsModel;
class ITheme;
class MCEditTool;
class ThemeManager;
class IconTheme;

namespace Meta {
class Index;
}

#if defined(APPLICATION)
#undef APPLICATION
#endif
#define APPLICATION (static_cast<Application*>(QCoreApplication::instance()))

// Used for checking if is a test
#if defined(APPLICATION_DYN)
#undef APPLICATION_DYN
#endif
#define APPLICATION_DYN (dynamic_cast<Application*>(QCoreApplication::instance()))

class Application : public QApplication {
    // friends for the purpose of limiting access to deprecated stuff
    Q_OBJECT
   public:
    enum Status { StartingUp, Failed, Succeeded, Initialized };

    enum Capability {
        None = 0,

        SupportsMSA = 1 << 0,
        SupportsFlame = 1 << 1,
        SupportsGameMode = 1 << 2,
        SupportsMangoHud = 1 << 3,
    };
    Q_DECLARE_FLAGS(Capabilities, Capability)

   public:
    Application(int& argc, char** argv);
    virtual ~Application();

    bool event(QEvent* event) override;

    SettingsObject* settings() const { return m_settings.get(); }

    qint64 timeSinceStart() const { return m_startTime.msecsTo(QDateTime::currentDateTime()); }

    QIcon logo();

    ThemeManager* themeManager() { return m_themeManager.get(); }

    ExternalUpdater* updater() { return m_updater.get(); }

    void triggerUpdateCheck();

    TranslationsModel* translations();

    JavaInstallList* javalist();

    InstanceList* instances() const { return m_instances.get(); }

    IconList* icons() const { return m_icons.get(); }

    MCEditTool* mcedit() const { return m_mcedit.get(); }

    AccountList* accounts() const { return m_accounts.get(); }

    Status status() const { return m_status; }

    const QMap<QString, std::shared_ptr<BaseProfilerFactory>>& profilers() const { return m_profilers; }

    void updateProxySettings(QString proxyTypeStr, QString addr, int port, QString user, QString password);

    QNetworkAccessManager* network();

    HttpMetaCache* metacache();

    Meta::Index* metadataIndex();

    void updateCapabilities();

    void detectLibraries();

    /*!
     * Finds and returns the full path to a jar file.
     * Returns a null-string if it could not be found.
     */
    QString getJarPath(QString jarFile);

    QString getMSAClientID();
    QString getFlameAPIKey();
    QString getModrinthAPIToken();
    QString getUserAgent();

    /// this is the root of the 'installation'. Used for automatic updates
    const QString& root() { return m_rootPath; }

    /// the data path the application is using
    const QString& dataRoot() { return m_dataPath; }

    /// the java installed path the application is using
    const QString javaPath();

    bool isPortable() { return m_portable; }

    const Capabilities capabilities() { return m_capabilities; }

    /*!
     * Opens a json file using either a system default editor, or, if not empty, the editor
     * specified in the settings
     */
    bool openJsonEditor(const QString& filename);

    InstanceWindow* showInstanceWindow(BaseInstance* instance, QString page = QString());
    MainWindow* showMainWindow(bool minimized = false);
    ViewLogWindow* showLogWindow();

    void updateIsRunning(bool running);
    bool updatesAreAllowed();

    void ShowGlobalSettings(class QWidget* parent, QString open_page = QString());

    bool updaterEnabled();
    QString updaterBinaryName();

    QUrl normalizeImportUrl(QString const& url);

   signals:
    void updateAllowedChanged(bool status);
    void globalSettingsAboutToOpen();
    void globalSettingsApplied();
    int currentCatChanged(int index);

    void oauthReplyRecieved(QVariantMap);

#ifdef Q_OS_MACOS
    void clickedOnDock();
#endif

   public slots:
    bool launch(BaseInstance* instance,
                bool online = true,
                bool demo = false,
                MinecraftTarget::Ptr targetToJoin = nullptr,
                MinecraftAccountPtr accountToUse = nullptr,
                const QString& offlineName = QString());
    bool kill(BaseInstance* instance);
    void closeCurrentWindow();

   private slots:
    void on_windowClose();
    void messageReceived(const QByteArray& message);
    void controllerFinished();
    void setupWizardFinished(int status);

   private:
    bool handleDataMigration(const QString& currentData, const QString& oldData, const QString& name, const QString& configFile) const;
    bool createSetupWizard();
    void performMainStartupAction();

    // sets the fatal error message and m_status to Failed.
    void showFatalErrorMessage(const QString& title, const QString& content);

   private:
    void addRunningInstance();
    void subRunningInstance();
    bool shouldExitNow() const;

   private:
    QHash<QString, int> m_qsaveResources;
    mutable QMutex m_qsaveResourcesMutex;

   private:
    QDateTime m_startTime;

    std::unique_ptr<QNetworkAccessManager> m_network;

    std::unique_ptr<ExternalUpdater> m_updater;
    std::unique_ptr<AccountList> m_accounts;

    std::unique_ptr<HttpMetaCache> m_metacache;
    std::unique_ptr<Meta::Index> m_metadataIndex;

    std::unique_ptr<SettingsObject> m_settings;
    std::unique_ptr<InstanceList> m_instances;
    std::unique_ptr<IconList> m_icons;
    std::unique_ptr<JavaInstallList> m_javalist;
    std::unique_ptr<TranslationsModel> m_translations;
    std::unique_ptr<GenericPageProvider> m_globalSettingsProvider;
    std::unique_ptr<MCEditTool> m_mcedit;
    QSet<QString> m_features;
    std::unique_ptr<ThemeManager> m_themeManager;

    QMap<QString, std::shared_ptr<BaseProfilerFactory>> m_profilers;

    QString m_rootPath;
    QString m_dataPath;
    Status m_status = Application::StartingUp;
    Capabilities m_capabilities;
    bool m_portable = false;

#ifdef Q_OS_MACOS
    Qt::ApplicationState m_prevAppState = Qt::ApplicationInactive;
#endif

#if defined Q_OS_WIN32
    // used on Windows to attach the standard IO streams
    bool consoleAttached = false;
#endif

    // FIXME: attach to instances instead.
    struct InstanceXtras {
        InstanceWindow* window = nullptr;
        std::unique_ptr<LaunchController> controller;
    };
    std::map<QString, InstanceXtras> m_instanceExtras;
    mutable QMutex m_instanceExtrasMutex;

    // main state variables
    size_t m_openWindows = 0;
    size_t m_runningInstances = 0;
    bool m_updateRunning = false;

    // main window, if any
    MainWindow* m_mainWindow = nullptr;

    // log window, if any
    ViewLogWindow* m_viewLogWindow = nullptr;

    // peer launcher instance connector - used to implement single instance launcher and signalling
    LocalPeer* m_peerInstance = nullptr;

    SetupWizard* m_setupWizard = nullptr;

   public:
    QString m_detectedGLFWPath;
    QString m_detectedOpenALPath;
    QString m_instanceIdToLaunch;
    QString m_serverToJoin;
    QString m_worldToJoin;
    QString m_profileToUse;
    bool m_offline = false;
    QString m_offlineName;
    bool m_liveCheck = false;
    QList<QUrl> m_urlsToImport;
    QString m_instanceIdToShowWindowOf;
    std::unique_ptr<QFile> logFile;
    std::unique_ptr<LogModel> logModel;

   public:
    void addQSavePath(QString);
    void removeQSavePath(QString);
    bool checkQSavePath(QString);
};
