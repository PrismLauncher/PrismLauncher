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

#include <QAbstractListModel>

#include <QString>
#include <QList>
#include <memory>

#include "OneSixLibrary.h"
#include "VersionFile.h"
#include "JarMod.h"

class OneSixInstance;

class InstanceVersion : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit InstanceVersion(OneSixInstance *instance, QObject *parent = 0);

	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex &parent) const;
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;

	void reload(const QStringList &external = QStringList());
	void clear();

	bool canRemove(const int index) const;

	QString versionFileId(const int index) const;

	// is this version unmodded vanilla minecraft?
	bool isVanilla();
	// remove any customizations on top of vanilla
	bool revertToVanilla();
	
	// does this version consist of obsolete files?
	bool hasDeprecatedVersionFiles();
	// remove obsolete files
	bool removeDeprecatedVersionFiles();
	
	// does this version have an FTB pack patch file?
	bool hasFtbPack();
	// remove FTB pack
	bool removeFtbPack();
	
	// does this version have any jar mods?
	bool hasJarMods();
	void installJarMods(QStringList selectedFiles);
	void installJarModByFilename(QString filepath);

	enum MoveDirection { MoveUp, MoveDown };
	void move(const int index, const MoveDirection direction);
	void resetOrder();

	// clears and reapplies all version files
	void reapply(const bool alreadyReseting = false);
	void finalize();

public
slots:
	bool remove(const int index);
	bool remove(const QString id);

public:
	QList<std::shared_ptr<OneSixLibrary>> getActiveNormalLibs();
	QList<std::shared_ptr<OneSixLibrary>> getActiveNativeLibs();

	static std::shared_ptr<InstanceVersion> fromJson(const QJsonObject &obj);

private:
	bool preremove(VersionPatchPtr patch);
	
	// data members
public:
	/// the ID - determines which jar to use! ACTUALLY IMPORTANT!
	QString id;

	/// the time this version was actually released by Mojang, as string and as QDateTime
	QString m_releaseTimeString;
	QDateTime m_releaseTime;

	/// the time this version was last updated by Mojang, as string and as QDateTime
	QString m_updateTimeString;
	QDateTime m_updateTime;

	/// Release type - "release" or "snapshot"
	QString type;
	/// Assets type - "legacy" or a version ID
	QString assets;
	/**
	 * DEPRECATED: Old versions of the new vanilla launcher used this
	 * ex: "username_session_version"
	 */
	QString processArguments;
	/// Same as above, but only for vanilla
	QString vanillaProcessArguments;
	/**
	 * arguments that should be used for launching minecraft
	 *
	 * ex: "--username ${auth_player_name} --session ${auth_session}
	 *      --version ${version_name} --gameDir ${game_directory} --assetsDir ${game_assets}"
	 */
	QString minecraftArguments;
	/// Same as above, but only for vanilla
	QString vanillaMinecraftArguments;
	/**
	 * the minimum launcher version required by this version ... current is 4 (at point of
	 * writing)
	 */
	int minimumLauncherVersion = 0xDEADBEEF;
	/**
	 * A list of all tweaker classes
	 */
	QStringList tweakers;
	/**
	 * The main class to load first
	 */
	QString mainClass;
	/**
	 * The applet class, for some very old minecraft releases
	 */
	QString appletClass;
	
	/// the list of libs - both active and inactive, native and java
	QList<OneSixLibraryPtr> libraries;

	/// same, but only vanilla.
	QList<OneSixLibraryPtr> vanillaLibraries;

	/// traits, collected from all the version files (version files can only add)
	QSet<QString> traits;

	/// A list of jar mods. version files can add those.
	QList<JarmodPtr> jarMods;

	/*
	FIXME: add support for those rules here? Looks like a pile of quick hacks to me though.

	"rules": [
		{
		"action": "allow"
		},
		{
		"action": "disallow",
		"os": {
			"name": "osx",
			"version": "^10\\.5\\.\\d$"
		}
		}
	],
	"incompatibilityReason": "There is a bug in LWJGL which makes it incompatible with OSX
	10.5.8. Please go to New Profile and use 1.5.2 for now. Sorry!"
	}
	*/
	// QList<Rule> rules;

	QList<VersionPatchPtr> VersionPatches;
	VersionPatchPtr versionPatch(const QString &id);
	VersionPatchPtr versionPatch(int index);

private:
	QStringList m_externalPatches;
	OneSixInstance *m_instance;
	void saveCurrentOrder() const;
	int getFreeOrderNumber();
};
