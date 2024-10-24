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
#include <java/JavaVersion.h>
#include <QDir>
#include <QProcess>
#include "BaseInstance.h"
#include "minecraft/launch/MinecraftTarget.h"
#include "minecraft/mod/Mod.h"

class ModFolderModel;
class ResourceFolderModel;
class ResourcePackFolderModel;
class ShaderPackFolderModel;
class TexturePackFolderModel;
class WorldList;
class GameOptions;
class LaunchStep;
class PackProfile;

class MinecraftInstance : public BaseInstance {
    Q_OBJECT
   public:
    MinecraftInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString& rootDir);
    virtual ~MinecraftInstance() = default;
    virtual void saveNow() override;

    void loadSpecificSettings() override;

    // FIXME: remove
    QString typeName() const override;
    // FIXME: remove
    QSet<QString> traits() const override;

    bool canEdit() const override { return true; }

    bool canExport() const override { return true; }

    void populateLaunchMenu(QMenu* menu) override;

    ////// Directories and files //////
    QString jarModsDir() const;
    QString resourcePacksDir() const;
    QString texturePacksDir() const;
    QString shaderPacksDir() const;
    QString modsRoot() const override;
    QString coreModsDir() const;
    QString nilModsDir() const;
    QString modsCacheLocation() const;
    QString libDir() const;
    QString worldDir() const;
    QString resourcesDir() const;
    QDir jarmodsPath() const;
    QDir librariesPath() const;
    QDir versionsPath() const;
    QString instanceConfigFolder() const override;

    // Path to the instance's minecraft directory.
    QString gameRoot() const override;

    // Path to the instance's minecraft bin directory.
    QString binRoot() const;

    // where to put the natives during/before launch
    QString getNativePath() const;

    // where the instance-local libraries should be
    QString getLocalLibraryPath() const;

    void updateRuntimeContext() override;

    //////  Profile management //////
    std::shared_ptr<PackProfile> getPackProfile() const;

    //////  Mod Lists  //////
    std::shared_ptr<ModFolderModel> loaderModList();
    std::shared_ptr<ModFolderModel> coreModList();
    std::shared_ptr<ModFolderModel> nilModList();
    std::shared_ptr<ResourcePackFolderModel> resourcePackList();
    std::shared_ptr<TexturePackFolderModel> texturePackList();
    std::shared_ptr<ShaderPackFolderModel> shaderPackList();
    std::shared_ptr<WorldList> worldList();
    std::shared_ptr<GameOptions> gameOptionsModel();

    //////  Launch stuff //////
    QList<Task::Ptr> createUpdateTask() override;
    shared_qobject_ptr<LaunchTask> createLaunchTask(AuthSessionPtr account, MinecraftTarget::Ptr targetToJoin) override;
    QStringList extraArguments() override;
    QStringList verboseDescription(AuthSessionPtr session, MinecraftTarget::Ptr targetToJoin) override;
    QList<Mod*> getJarMods() const;
    QString createLaunchScript(AuthSessionPtr session, MinecraftTarget::Ptr targetToJoin);
    /// get arguments passed to java
    QStringList javaArguments();
    QString getLauncher();
    bool shouldApplyOnlineFixes();

    /// get variables for launch command variable substitution/environment
    QMap<QString, QString> getVariables() override;

    /// create an environment for launching processes
    QProcessEnvironment createEnvironment() override;
    QProcessEnvironment createLaunchEnvironment() override;

    /// guess log level from a line of minecraft log
    MessageLevel::Enum guessLevel(const QString& line, MessageLevel::Enum level) override;

    IPathMatcher::Ptr getLogFileMatcher() override;

    QString getLogFileRoot() override;

    QString getStatusbarDescription() override;

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
    QMap<QString, QString> createCensorFilterFromSession(AuthSessionPtr session);

   protected:  // data
    std::shared_ptr<PackProfile> m_components;
    mutable std::shared_ptr<ModFolderModel> m_loader_mod_list;
    mutable std::shared_ptr<ModFolderModel> m_core_mod_list;
    mutable std::shared_ptr<ModFolderModel> m_nil_mod_list;
    mutable std::shared_ptr<ResourcePackFolderModel> m_resource_pack_list;
    mutable std::shared_ptr<ShaderPackFolderModel> m_shader_pack_list;
    mutable std::shared_ptr<TexturePackFolderModel> m_texture_pack_list;
    mutable std::shared_ptr<WorldList> m_world_list;
    mutable std::shared_ptr<GameOptions> m_game_options;
};

using MinecraftInstancePtr = std::shared_ptr<MinecraftInstance>;
