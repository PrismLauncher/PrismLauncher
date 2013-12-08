/* Copyright 2013 MultiMC Contributors
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

#include <QStringList>
#include <QDir>

#include "BaseInstance.h"

class OneSixVersion;
class Task;
class ModList;

class OneSixInstance : public BaseInstance
{
	Q_OBJECT
public:
	explicit OneSixInstance(const QString &rootDir, SettingsObject *settings,
							QObject *parent = 0);

	//////  Mod Lists  //////
	std::shared_ptr<ModList> loaderModList();
	std::shared_ptr<ModList> resourcePackList();

	////// Directories //////
	QString resourcePacksDir() const;
	QString loaderModsDir() const;
	virtual QString instanceConfigFolder() const override;

	virtual std::shared_ptr<Task> doUpdate(bool only_prepare) override;
	virtual MinecraftProcess *prepareForLaunch(MojangAccountPtr account) override;

	virtual void cleanupAfterRun() override;

	virtual QString intendedVersionId() const override;
	virtual bool setIntendedVersionId(QString version) override;

	virtual QString currentVersionId() const override;

	virtual bool shouldUpdate() const override;
	virtual void setShouldUpdate(bool val) override;

	virtual QDialog *createModEditDialog(QWidget *parent) override;

	/// reload the full version json file. return true on success!
	bool reloadFullVersion();
	/// get the current full version info
	std::shared_ptr<OneSixVersion> getFullVersion();
	/// revert the current custom version back to base
	bool revertCustomVersion();
	/// customize the current base version
	bool customizeVersion();
	/// is the current version original, or custom?
	virtual bool versionIsCustom() override;

	virtual QString defaultBaseJar() const override;
	virtual QString defaultCustomBaseJar() const override;

	virtual bool menuActionEnabled(QString action_name) const override;
	virtual QString getStatusbarDescription() override;

private:
	QStringList processMinecraftArgs(MojangAccountPtr account);
	QDir reconstructAssets(std::shared_ptr<OneSixVersion> version);
};
