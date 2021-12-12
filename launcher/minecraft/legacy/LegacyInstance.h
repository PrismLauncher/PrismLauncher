/* Copyright 2013-2021 MultiMC Contributors
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

#pragma once

#include "BaseInstance.h"
#include "launch/LaunchTask.h"

class ModFolderModel;
class LegacyModList;
class WorldList;
class Task;
/*
 * WHY: Legacy instances - from MultiMC 3 and 4 - are here only to provide a way to upgrade them to the current format.
 */
class LegacyInstance : public BaseInstance
{
    Q_OBJECT
public:

    explicit LegacyInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir);

    virtual void saveNow() override {}

    /// Path to the instance's minecraft.jar
    QString runnableJar() const;

    //! Path to the instance's modlist file.
    QString modListFile() const;

    ////// Directories //////
    QString libDir() const;
    QString savesDir() const;
    QString texturePacksDir() const;
    QString jarModsDir() const;
    QString coreModsDir() const;
    QString resourceDir() const;

    QString instanceConfigFolder() const override;

    QString gameRoot() const override; // Path to the instance's minecraft directory.
    QString modsRoot() const override; // Path to the instance's minecraft directory.
    QString binRoot() const; // Path to the instance's minecraft bin directory.

    /// Get the curent base jar of this instance. By default, it's the
    /// versions/$version/$version.jar
    QString baseJar() const;

    /// the default base jar of this instance
    QString defaultBaseJar() const;
    /// the default custom base jar of this instance
    QString defaultCustomBaseJar() const;

    // the main jar that we actually want to keep when migrating the instance
    QString mainJarToPreserve() const;

    /*!
     * Whether or not custom base jar is used
     */
    bool shouldUseCustomBaseJar() const;

    /*!
     * The value of the custom base jar
     */
    QString customBaseJar() const;

    std::shared_ptr<LegacyModList> jarModList() const;
    std::shared_ptr<WorldList> worldList() const;

    /*!
     * Whether or not the instance's minecraft.jar needs to be rebuilt.
     * If this is true, when the instance launches, its jar mods will be
     * re-added to a fresh minecraft.jar file.
     */
    bool shouldRebuild() const;

    QString currentVersionId() const;
    QString intendedVersionId() const;

    QSet<QString> traits() const override
    {
        return {"legacy-instance", "texturepacks"};
    };

    virtual bool shouldUpdate() const;
    virtual Task::Ptr createUpdateTask(Net::Mode mode) override;

    virtual QString typeName() const override;

    bool canLaunch() const override
    {
        return false;
    }
    bool canEdit() const override
    {
        return true;
    }
    bool canExport() const override
    {
        return false;
    }
    shared_qobject_ptr<LaunchTask> createLaunchTask(
            AuthSessionPtr account, MinecraftServerTargetPtr serverToJoin) override
    {
        return nullptr;
    }
    IPathMatcher::Ptr getLogFileMatcher() override
    {
        return nullptr;
    }
    QString getLogFileRoot() override
    {
        return gameRoot();
    }

    QString getStatusbarDescription() override;
    QStringList verboseDescription(AuthSessionPtr session, MinecraftServerTargetPtr serverToJoin) override;

    QProcessEnvironment createEnvironment() override
    {
        return QProcessEnvironment();
    }
    QMap<QString, QString> getVariables() const override
    {
        return {};
    }
protected:
    mutable std::shared_ptr<LegacyModList> jar_mod_list;
    mutable std::shared_ptr<WorldList> m_world_list;
};
