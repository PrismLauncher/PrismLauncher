// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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
#include <cassert>
#include <cstdint>

#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QList>
#include <QMenu>
#include <QObject>
#include <QProcess>
#include <QSet>

#include <java/JavaVersion.h>
#include <minecraft/mod/DataPackFolderModel.h>

#include "RuntimeContext.h"
#include "minecraft/auth/MinecraftAccount.h"
#include "minecraft/launch/MinecraftTarget.h"
#include "minecraft/mod/Mod.h"
#include "settings/SettingsObject.h"

class Task;
class LaunchTask;
class ModFolderModel;
class ResourceFolderModel;
class ResourcePackFolderModel;
class ShaderPackFolderModel;
class TexturePackFolderModel;
class WorldList;
class LaunchStep;
class LaunchProfile;
class PackProfile;

/// Shortcut saving target representations
enum class ShortcutTarget : std::uint8_t { Desktop, Applications, Other };

/// Shortcut data representation
struct ShortcutData {
    QString name;
    QString filePath;
    ShortcutTarget target = ShortcutTarget::Other;
};

/// Console settings
int getConsoleMaxLines(SettingsObject* settings);
bool shouldStopOnConsoleOverflow(SettingsObject* settings);

class MinecraftInstance : public QObject {
    Q_OBJECT
   public: /* types */
    enum class Status : std::uint8_t {
        Present,
        Gone  // either nuked or invalidated
    };

   public:
    MinecraftInstance(SettingsObject* globalSettings, std::unique_ptr<SettingsObject> settings, const QString& rootDir);
    ~MinecraftInstance() override;

    void saveNow();

    /***
     * the instance has been invalidated - it is no longer tracked by the launcher for some reason,
     * but it has not necessarily been deleted.
     *
     * Happens when the instance folder changes to some other location, or the instance is removed by external means.
     */
    void invalidate();

    /// The instance's ID. The ID SHALL be determined by LAUNCHER internally. The ID IS guaranteed to
    /// be unique.
    QString id() const;
    QString uuid() const { return m_uuid; }
    void regenerateUuid();

    void setMinecraftRunning(bool running);
    void setRunning(bool running);
    bool isRunning() const;
    int64_t totalTimePlayed() const;
    int64_t lastTimePlayed() const;
    bool countTimePlayed() const;
    void resetTimePlayed();

    /// get the type of this instance
    QString instanceType() const;

    /// Path to the instance's root directory.
    QString instanceRoot() const;

    QString name() const;
    void setName(const QString& val);

    /// Sync name and rename instance dir accordingly; returns true if successful
    bool syncInstanceDirName(const QString& newRoot) const;

    /// Register a created shortcut
    void registerShortcut(const ShortcutData& data);
    QList<ShortcutData> shortcuts() const;
    void setShortcuts(const QList<ShortcutData>& shortcuts);

    /// Value used for instance window titles
    QString windowTitle() const;

    QString iconKey() const;
    void setIconKey(const QString& val);

    QString notes() const;
    void setNotes(const QString& val);

    QString getPreLaunchCommand();
    QString getPostExitCommand();
    QString getWrapperCommand();

    bool isManagedPack() const;
    QString getManagedPackType() const;
    QString getManagedPackID() const;
    QString getManagedPackName() const;
    QString getManagedPackVersionID() const;
    QString getManagedPackVersionName() const;
    void setManagedPack(const QString& type, const QString& id, const QString& name, const QString& versionId, const QString& version);

    /**
     * Gets the time that the instance was last launched.
     * Stored in milliseconds since epoch.
     */
    qint64 lastLaunch() const;
    /// Sets the last launched time to 'val' milliseconds since epoch
    void setLastLaunch(qint64 val = QDateTime::currentMSecsSinceEpoch());

    /*!
     * \brief Gets this instance's settings object.
     * This settings object stores instance-specific settings.
     *
     * Note that this method is not const.
     * It may call loadSpecificSettings() to ensure those are loaded.
     *
     * \return A pointer to this instance's settings object.
     */
    SettingsObject* settings();

    void loadSpecificSettings();

    // FIXME: remove
    QSet<QString> traits() const;

    void populateLaunchMenu(QMenu* menu);

    /// returns the current launch task (if any)
    LaunchTask* getLaunchTask();

    bool hasVersionBroken() const { return m_hasBrokenVersion; }
    void setVersionBroken(bool value)
    {
        if (m_hasBrokenVersion != value) {
            m_hasBrokenVersion = value;
            emit propertiesChanged();
        }
    }

    bool hasUpdateAvailable() const { return m_hasUpdate; }
    void setUpdateAvailable(bool value)
    {
        if (m_hasUpdate != value) {
            m_hasUpdate = value;
            emit propertiesChanged();
        }
    }

    bool hasCrashed() const { return m_crashed; }
    void setCrashed(bool value)
    {
        if (m_crashed != value) {
            m_crashed = value;
            emit propertiesChanged();
        }
    }

    bool canLaunch() const;

    bool reloadSettings();

    Status currentStatus() const;

    QStringList getLinkedInstances() const;
    void setLinkedInstances(const QStringList& list);
    void addLinkedInstanceId(const QString& id);
    bool removeLinkedInstanceId(const QString& id);
    bool isLinkedToInstanceId(const QString& id) const;

    bool isLegacy() const;

    ////// Directories and files //////
    QString jarModsDir() const;
    QString resourcePacksDir() const;
    QString texturePacksDir() const;
    QString shaderPacksDir() const;
    QString modsRoot() const;
    QString coreModsDir() const;
    QString nilModsDir() const;
    QString dataPacksDir();
    QString modsCacheLocation() const;
    QString libDir() const;
    QString worldDir() const;
    QString resourcesDir() const;
    QDir jarmodsPath() const;
    QDir librariesPath() const;
    QDir versionsPath() const;
    QString instanceConfigFolder() const;

    // Path to the instance's minecraft directory.
    QString gameRoot() const;

    // Path to the instance's minecraft bin directory.
    QString binRoot() const;

    // where to put the natives during/before launch
    QString getNativePath() const;

    // where the instance-local libraries should be
    QString getLocalLibraryPath() const;

    /** Returns whether the instance, with its version, has support for demo mode. */
    bool supportsDemo() const;

    void updateRuntimeContext();
    RuntimeContext runtimeContext() const { return m_runtimeContext; }

    //////  Profile management //////
    PackProfile* getPackProfile() const;

    //////  Mod Lists  //////
    ModFolderModel* loaderModList();
    ModFolderModel* coreModList();
    ModFolderModel* nilModList();
    ResourcePackFolderModel* resourcePackList();
    TexturePackFolderModel* texturePackList();
    ShaderPackFolderModel* shaderPackList();
    DataPackFolderModel* dataPackList();
    QList<ResourceFolderModel*> resourceLists();
    WorldList* worldList();

    //////  Launch stuff //////
    QList<Task::Ptr> createUpdateTask();
    LaunchTask* createLaunchTask(AuthSessionPtr account, MinecraftTarget::Ptr targetToJoin);
    QStringList extraArguments();
    /**
     * 'print' a verbose description of the instance into a QStringList
     */
    QStringList verboseDescription(AuthSessionPtr session, MinecraftTarget::Ptr targetToJoin);
    QList<Mod*> getJarMods() const;
    QString createLaunchScript(AuthSessionPtr session, MinecraftTarget::Ptr targetToJoin);
    /// get arguments passed to java
    QStringList javaArguments();
    QString getLauncher();
    bool shouldApplyOnlineFixes();

    /// get variables for launch command variable substitution/environment
    QMap<QString, QString> getVariables();

    /// create an environment for launching processes
    QProcessEnvironment createEnvironment();
    QProcessEnvironment createLaunchEnvironment();

    /*!
     * Returns the root folder to use for looking up log files
     */
    QStringList getLogFileSearchPaths();

    QString getStatusbarDescription();

    // FIXME: remove
    virtual QStringList getClassPath();
    // FIXME: remove
    virtual QStringList getNativeJars();
    // FIXME: remove
    virtual QString getMainClass() const;

    // FIXME: remove
    virtual QStringList processMinecraftArgs(AuthSessionPtr account, MinecraftTarget::Ptr targetToJoin) const;

    virtual JavaVersion getJavaVersion();

   protected:
    void changeStatus(Status newStatus);

    SettingsObject* globalSettings() const { return m_globalSettings; }

    bool isSpecificSettingsLoaded() const { return m_specificSettingsLoaded; }
    void setSpecificSettingsLoaded(bool loaded) { m_specificSettingsLoaded = loaded; }

    QMap<QString, QString> createCensorFilterFromSession(AuthSessionPtr session);
    QMap<QString, QString> makeProfileVarMapping(std::shared_ptr<LaunchProfile> profile) const;

   signals:
    /*!
     * \brief Signal emitted when properties relevant to the instance view change
     */
    void propertiesChanged();

    void launchTaskChanged(LaunchTask*);

    void runningStatusChanged(bool running);

    void profilerChanged();

    void statusChanged(Status from, Status to);

   protected slots:
    void iconUpdated(const QString& key);

   protected:  // data
    QString m_rootDir;
    std::unique_ptr<SettingsObject> m_settings;
    bool m_isRunning = false;
    std::unique_ptr<LaunchTask> m_launchProcess;
    QDateTime m_timeStarted;
    RuntimeContext m_runtimeContext;

    std::unique_ptr<PackProfile> m_components;
    std::unique_ptr<ModFolderModel> m_loader_mod_list;
    std::unique_ptr<ModFolderModel> m_core_mod_list;
    std::unique_ptr<ModFolderModel> m_nil_mod_list;
    std::unique_ptr<ResourcePackFolderModel> m_resource_pack_list;
    std::unique_ptr<ShaderPackFolderModel> m_shader_pack_list;
    std::unique_ptr<TexturePackFolderModel> m_texture_pack_list;
    std::unique_ptr<DataPackFolderModel> m_data_pack_list;
    std::unique_ptr<WorldList> m_world_list;

   private:  // data
    QString m_uuid;
    Status m_status = Status::Present;
    bool m_crashed = false;
    bool m_hasUpdate = false;
    bool m_hasBrokenVersion = false;

    SettingsObject* m_globalSettings;
    bool m_specificSettingsLoaded = false;
};
