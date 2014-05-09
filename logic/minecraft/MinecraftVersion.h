/* Copyright 2013 Andrew Okin
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

#include "logic/BaseVersion.h"
#include "VersionPatch.h"
#include "VersionFile.h"

class VersionFinal;

struct MinecraftVersion : public BaseVersion, public VersionPatch
{
	/// The URL that this version will be downloaded from. maybe.
	QString download_url;

	/// is this the latest version?
	bool is_latest = false;

	/// is this a snapshot?
	bool is_snapshot = false;

	/// where is this from?
	enum VersionSource
	{
		Builtin,
		Mojang
	} m_versionSource = Builtin;

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

	/// order of this file... default = -2
	int order = -2;

	bool usesLegacyLauncher();
	virtual QString descriptor() override;
	virtual QString name() override;
	virtual QString typeString() const override;
	virtual bool hasJarMods() override;
	virtual bool isVanilla() override;
	virtual void applyTo(VersionFinal *version) override;
	virtual int getOrder();
	virtual void setOrder(int order);
	virtual QList<JarmodPtr> getJarMods() override;
	virtual QString getPatchID()
	{
		return "net.minecraft";
	}
	virtual QString getPatchVersion()
	{
		return m_descriptor;
	}
	virtual QString getPatchName()
	{
		return "Minecraft";
	}
	virtual QString getPatchFilename()
	{
		return QString();
	};
};
