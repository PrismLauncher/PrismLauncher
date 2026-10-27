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

#include <memory>

#include <QFlag>
#include <QMap>
#include <QObject>
#include <QVariant>

#include "LaunchMode.h"
#include "QObjectPtr.h"

class AccountList;
class BaseInstance;
class BaseProfilerFactory;
class ExternalUpdater;
class HttpMetaCache;
class IconList;
class InstanceList;
class InstanceWindow;
class JavaInstallList;
class LogModel;
class MainWindow;
class MinecraftAccount;
class MinecraftInstance;
struct MinecraftTarget;
class QFile;
class QIcon;
class QNetworkAccessManager;
class QUrl;
class SettingsObject;
class ThemeManager;
class TranslationsModel;
class ViewLogWindow;

namespace Meta {
class Index;
}

#if defined(APPLICATION)
#undef APPLICATION
#endif
#define APPLICATION Application::get()

class Application : public QObject {
    Q_OBJECT
   public:
    enum Capability : std::uint8_t {
        None = 0U,

        SupportsMSA = 1U << 0U,
        SupportsFlame = 1U << 1U,
        SupportsGameMode = 1U << 2U,
        SupportsMangoHud = 1U << 3U,
    };
    Q_DECLARE_FLAGS(Capabilities, Capability)

    Application();
    ~Application() override;

    static Application* get() { return s_instance; }

    int exec(int& argc, char** argv);

    bool eventFilter(QObject* watched, QEvent* event) override;

    SettingsObject* settings() const;
    SettingsObject* playtimeSettings() const;

    static QIcon logo();

    ThemeManager* themeManager();

    ExternalUpdater* updater();

    void triggerUpdateCheck();

    TranslationsModel* translations();

    JavaInstallList* javalist();

    InstanceList* instances() const;

    IconList* icons() const;

    AccountList* accounts() const;

    const QMap<QString, std::shared_ptr<BaseProfilerFactory>>& profilers() const;

    void updateProxySettings(const QString& proxyTypeStr, const QString& addr, int port, const QString& user, const QString& password);

    QNetworkAccessManager* network();

    HttpMetaCache* metacache();

    Meta::Index* metadataIndex();

    void updateCapabilities();

    void detectLibraries();

    /*!
     * Finds and returns the full path to a jar file.
     * Returns a null-string if it could not be found.
     */
    QString getJarPath(const QString& jarFile);

    QString getMSAClientID();
    QString getFlameAPIKey();
    QString getModrinthAPIToken();
    QString getUserAgent();

    /// this is the root of the 'installation'. Used for automatic updates
    const QString& root() const;

    /// the data path the application is using
    const QString& dataRoot() const;

    /// the java installed path the application is using
    QString javaPath();

    bool isPortable() const;

    Capabilities capabilities() const;

    /*!
     * Opens a json file using either a system default editor, or, if not empty, the editor
     * specified in the settings
     */
    bool openJsonEditor(const QString& filename);

    InstanceWindow* showInstanceWindow(MinecraftInstance* instance, const QString& page = QString());
    MainWindow* showMainWindow(bool minimized = false);
    ViewLogWindow* showLogWindow();

    void updateIsRunning(bool running);
    bool updatesAreAllowed() const;

    void ShowGlobalSettings(class QWidget* parent, QString openPage = QString());

    bool updaterEnabled();
    static QString updaterBinaryName();

    static QUrl normalizeImportUrl(const QString& url);

    bool launch(MinecraftInstance* instance,
                LaunchMode mode,
                std::shared_ptr<MinecraftTarget> targetToJoin,
                shared_qobject_ptr<MinecraftAccount> accountToUse,
                const QString& offlineName = {});
    bool kill(BaseInstance* instance);

    void addQSavePath(const QString&);
    void removeQSavePath(const QString&);
    bool checkQSavePath(const QString&);

    QFile* logFile();

    LogModel* logModel();

    const QString& detectedGLFWPath() const;

    const QString& detectedOpenALPath() const;

    const QString& detectedSDLPath() const;

   signals:
    void updateAllowedChanged(bool status);
    void globalSettingsAboutToOpen();
    void globalSettingsApplied();
    int currentCatChanged(int index);

    void oauthReplyRecieved(QVariantMap);

#ifdef Q_OS_MACOS
    void clickedOnDock();
#endif

   private slots:
    void on_windowClose();
    void messageReceived(const QByteArray& message);
    void controllerFinished();

   private:
    bool createSetupWizard();
    void performMainStartupAction();

   private:
    void addRunningInstance();
    void subRunningInstance();
    bool shouldExitNow() const;

   private:
    static inline Application* s_instance = nullptr;

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
