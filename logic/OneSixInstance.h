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

#include "BaseInstance.h"

#include "logic/minecraft/InstanceVersion.h"
#include "logic/ModList.h"
#include "gui/pages/BasePageProvider.h"

class OneSixInstance : public BaseInstance, public BasePageProvider
{
	Q_OBJECT
public:
	explicit OneSixInstance(const QString &rootDir, SettingsObject *settings,
						  QObject *parent = 0);
	virtual ~OneSixInstance(){};

	virtual void init() override;

	////// Edit Instance Dialog stuff //////
	virtual QList<BasePage *> getPages();
	virtual QString dialogTitle();

	//////  Mod Lists  //////
	std::shared_ptr<ModList> loaderModList();
	std::shared_ptr<ModList> coreModList();
	std::shared_ptr<ModList> resourcePackList() override;
	std::shared_ptr<ModList> texturePackList() override;

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
	virtual bool prepareForLaunch(AuthSessionPtr account, QString & launchScript) override;

	virtual void cleanupAfterRun() override;

	virtual QString intendedVersionId() const override;
	virtual bool setIntendedVersionId(QString version) override;

	virtual QString currentVersionId() const override;

	virtual bool shouldUpdate() const override;
	virtual void setShouldUpdate(bool val) override;

	/**
	 * reload the full version json files.
	 *
	 * throws various exceptions :3
	 */
	void reloadVersion();

	/// clears all version information in preparation for an update
	void clearVersion();

	/// get the current full version info
	std::shared_ptr<InstanceVersion> getFullVersion() const;

	/// is the current version original, or custom?
	virtual bool versionIsCustom() override;

	/// does this instance have an FTB pack patch inside?
	bool versionIsFTBPack();

	virtual QString getStatusbarDescription() override;

	virtual QDir jarmodsPath() const;
	virtual QDir librariesPath() const;
	virtual QDir versionsPath() const;
	virtual QStringList externalPatches() const;
	virtual bool providesVersionFile() const;

	bool reload() override;

	virtual QStringList extraArguments() const override;

	std::shared_ptr<OneSixInstance> getSharedPtr();

signals:
	void versionReloaded();

private:
	QStringList processMinecraftArgs(AuthSessionPtr account);
	QDir reconstructAssets(std::shared_ptr<InstanceVersion> version);

protected:
	std::shared_ptr<InstanceVersion> version;
	std::shared_ptr<ModList> jar_mod_list;
	std::shared_ptr<ModList> loader_mod_list;
	std::shared_ptr<ModList> core_mod_list;
	std::shared_ptr<ModList> resource_pack_list;
	std::shared_ptr<ModList> texture_pack_list;
};

Q_DECLARE_METATYPE(std::shared_ptr<OneSixInstance>)
