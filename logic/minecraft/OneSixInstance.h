/* Copyright 2013-2015 MultiMC Contributors
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

#include "minecraft/MinecraftInstance.h"

#include "minecraft/MinecraftProfile.h"
#include "minecraft/ModList.h"

class OneSixInstance : public MinecraftInstance
{
	Q_OBJECT
public:
	explicit OneSixInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString &rootDir);
	virtual ~OneSixInstance(){};

	virtual void init();

	//////  Mod Lists  //////
	std::shared_ptr<ModList> loaderModList() const;
	std::shared_ptr<ModList> coreModList() const;
	std::shared_ptr<ModList> resourcePackList() const override;
	std::shared_ptr<ModList> texturePackList() const override;
	virtual QList<Mod> getJarMods() const override;
	virtual void createProfile();

	virtual QSet<QString> traits();

	////// Directories and files //////
	QString jarModsDir() const;
	QString resourcePacksDir() const;
	QString texturePacksDir() const;
	QString loaderModsDir() const;
	QString coreModsDir() const;
	QString libDir() const;
	virtual QString instanceConfigFolder() const override;

	virtual std::shared_ptr<Task> doUpdate() override;
	virtual BaseProcess *prepareForLaunch(AuthSessionPtr account) override;

	virtual void cleanupAfterRun() override;

	virtual QString intendedVersionId() const override;
	virtual bool setIntendedVersionId(QString version) override;

	virtual QString currentVersionId() const override;

	virtual bool shouldUpdate() const override;
	virtual void setShouldUpdate(bool val) override;

	/**
	 * reload the profile, including version json files.
	 *
	 * throws various exceptions :3
	 */
	void reloadProfile();

	/// clears all version information in preparation for an update
	void clearProfile();

	/// get the current full version info
	std::shared_ptr<MinecraftProfile> getMinecraftProfile() const;

	virtual QString getStatusbarDescription() override;

	virtual QDir jarmodsPath() const;
	virtual QDir librariesPath() const;
	virtual QDir versionsPath() const;
	virtual bool providesVersionFile() const;

	bool reload() override;

	virtual QStringList extraArguments() const override;

	std::shared_ptr<OneSixInstance> getSharedPtr();

signals:
	void versionReloaded();

private:
	QStringList processMinecraftArgs(AuthSessionPtr account);

protected:
	std::shared_ptr<MinecraftProfile> m_version;
	mutable std::shared_ptr<ModList> m_loader_mod_list;
	mutable std::shared_ptr<ModList> m_core_mod_list;
	mutable std::shared_ptr<ModList> m_resource_pack_list;
	mutable std::shared_ptr<ModList> m_texture_pack_list;
};

Q_DECLARE_METATYPE(std::shared_ptr<OneSixInstance>)
