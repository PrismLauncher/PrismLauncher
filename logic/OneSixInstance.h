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

#include "BaseInstance.h"

class OneSixVersion;
class BaseUpdate;
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
	virtual QString instanceConfigFolder() const;

	virtual BaseUpdate *doUpdate();
	virtual MinecraftProcess *prepareForLaunch(MojangAccountPtr account);
	virtual void cleanupAfterRun();

	virtual QString intendedVersionId() const;
	virtual bool setIntendedVersionId(QString version);

	virtual QString currentVersionId() const;
	// virtual void setCurrentVersionId ( QString val ) {};

	virtual bool shouldUpdate() const;
	virtual void setShouldUpdate(bool val);

	virtual QDialog *createModEditDialog(QWidget *parent);

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

	virtual QString defaultBaseJar() const;
	virtual QString defaultCustomBaseJar() const;

	virtual bool menuActionEnabled(QString action_name) const;
	virtual QString getStatusbarDescription();

private:
	QStringList processMinecraftArgs(MojangAccountPtr account);
};
