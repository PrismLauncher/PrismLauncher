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

#include <QStringList>
#include <QSet>
#include <QDateTime>

#include "BaseVersion.h"
#include "ProfilePatch.h"
#include "VersionFile.h"
#include "VersionSource.h"

#include "multimc_logic_export.h"

class MinecraftProfile;
class MinecraftVersion;
typedef std::shared_ptr<MinecraftVersion> MinecraftVersionPtr;

class MULTIMC_LOGIC_EXPORT MinecraftVersion : public BaseVersion, public ProfilePatch
{
public: /* methods */
	bool usesLegacyLauncher();
	virtual QString descriptor() override;
	virtual QString name() override;
	virtual QString typeString() const override;
	virtual bool hasJarMods() override;
	virtual bool isMinecraftVersion() override;
	virtual void applyTo(MinecraftProfile *version) override;
	virtual int getOrder() override;
	virtual void setOrder(int order) override;
	virtual QList<JarmodPtr> getJarMods() override;
	virtual QString getPatchID() override;
	virtual QString getPatchVersion() override;
	virtual QString getPatchName() override;
	virtual QString getPatchFilename() override;
	bool needsUpdate();
	bool hasUpdate();
	virtual bool isCustom() override;
	virtual bool isMoveable() override
	{
		return false;
	}
	virtual bool isCustomizable() override;
	virtual bool isRemovable() override
	{
		return false;
	}
	virtual bool isRevertible() override
	{
		return false;
	}
	virtual bool isEditable() override
	{
		return false;
	}
	virtual bool isVersionChangeable() override
	{
		return true;
	}

	VersionFilePtr getVersionFile();

	virtual QJsonDocument toJson(bool saveOrder) override;

	QString getUrl() const;

private: /* methods */
	void applyFileTo(MinecraftProfile *version);

public: /* data */
	VersionSource m_versionSource = Builtin;

	/// The URL that this version will be downloaded from.
	QString m_versionFileURL;

	/// the human readable version name
	QString m_name;

	/// the version ID.
	QString m_descriptor;

	/// version traits. added by MultiMC
	QSet<QString> m_traits;

	/// The main class this version uses (if any, can be empty).
	QString m_mainClass;

	/// The applet class this version uses (if any, can be empty).
	QString m_appletClass;

	/// The process arguments used by this version
	QString m_processArguments;

	/// The type of this release
	QString m_type;

	/// the time this version was actually released by Mojang, as string and as QDateTime
	QString m_releaseTimeString;
	QDateTime m_releaseTime;

	/// the time this version was last updated by Mojang, as string and as QDateTime
	QString m_updateTimeString;
	QDateTime m_updateTime;

	/// MD5 hash of the minecraft jar
	QString m_jarChecksum;

	/// order of this file... default = -2
	int order = -2;

	/// an update available from Mojang
	MinecraftVersionPtr upstreamUpdate;
};
