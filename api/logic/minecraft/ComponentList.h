/* Copyright 2013-2017 MultiMC Contributors
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

#include "Library.h"
#include "LaunchProfile.h"
#include "ProfilePatch.h"
#include "ProfileUtils.h"
#include "BaseVersion.h"
#include "MojangDownloadInfo.h"
#include "multimc_logic_export.h"

class MinecraftInstance;


class MULTIMC_LOGIC_EXPORT ComponentList : public QAbstractListModel
{
	Q_OBJECT

public:
	explicit ComponentList(MinecraftInstance * instance);
	virtual ~ComponentList();

	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	virtual int columnCount(const QModelIndex &parent) const override;
	virtual Qt::ItemFlags flags(const QModelIndex &index) const override;

	/// is this version unchanged by the user?
	bool isVanilla();

	/// remove any customizations on top of whatever 'vanilla' means
	bool revertToVanilla();

	/// install more jar mods
	void installJarMods(QStringList selectedFiles);

	/// install a jar/zip as a replacement for the main jar
	void installCustomJar(QString selectedFile);

	/// DEPRECATED, remove ASAP
	int getFreeOrderNumber();

	enum MoveDirection { MoveUp, MoveDown };
	/// move patch file # up or down the list
	void move(const int index, const MoveDirection direction);

	/// remove patch file # - including files/records
	bool remove(const int index);

	/// remove patch file by id - including files/records
	bool remove(const QString id);

	bool customize(int index);

	bool revertToBase(int index);

	void resetOrder();

	/// reload all profile patches from storage, clear the profile and apply the patches
	void reload();

	/// apply the patches. Catches all the errors and returns true/false for success/failure
	bool reapplyPatches();

	std::shared_ptr<LaunchProfile> getProfile() const;
	void clearProfile();
public:
	/// get the profile patch by id
	ProfilePatchPtr versionPatch(const QString &id);

	/// get the profile patch by index
	ProfilePatchPtr versionPatch(int index);

	/// save the current patch order
	void saveCurrentOrder() const;

	/// Remove all the patches
	void clearPatches();

	/// Add the patch object to the internal list of patches
	void appendPatch(ProfilePatchPtr patch);

private:
	void load_internal();
	bool resetOrder_internal();
	bool saveOrder_internal(ProfileUtils::PatchOrder order) const;
	bool installJarMods_internal(QStringList filepaths);
    bool installCustomJar_internal(QString filepath);
	bool removePatch_internal(ProfilePatchPtr patch);
	bool customizePatch_internal(ProfilePatchPtr patch);
	bool revertPatch_internal(ProfilePatchPtr patch);
	void loadDefaultBuiltinPatches_internal();
	void loadUserPatches_internal();
	void upgradeDeprecatedFiles_internal();

private: /* data */
	/// list of attached profile patches
	QList<ProfilePatchPtr> m_patches;

	// the instance this belongs to
	MinecraftInstance *m_instance;

	std::shared_ptr<LaunchProfile> m_profile;
};
