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

#include <QObject>
#include <QList>
#include <QSet>

#include "BaseVersionList.h"
#include "tasks/Task.h"
#include "minecraft/MinecraftVersion.h"
#include <net/NetJob.h>

#include "multimc_logic_export.h"

class MCVListLoadTask;
class MCVListVersionUpdateTask;

class MULTIMC_LOGIC_EXPORT MinecraftVersionList : public BaseVersionList
{
	Q_OBJECT
private:
	void sortInternal();
	void loadBuiltinList();
	void loadMojangList(QJsonDocument jsonDoc, VersionSource source);
	void loadCachedList();
	void saveCachedList();
	void finalizeUpdate(QString version);
public:
	friend class MCVListLoadTask;
	friend class MCVListVersionUpdateTask;

	explicit MinecraftVersionList(QObject *parent = 0);

	std::shared_ptr<Task> createUpdateTask(QString version);

	virtual Task *getLoadTask() override;
	virtual bool isLoaded() override;
	virtual const BaseVersionPtr at(int i) const override;
	virtual int count() const override;
	virtual void sortVersions() override;
	virtual QVariant data(const QModelIndex & index, int role) const override;
	virtual RoleList providesRoles() const override;

	virtual BaseVersionPtr getLatestStable() const override;
	virtual BaseVersionPtr getRecommended() const override;

protected:
	QList<BaseVersionPtr> m_vlist;
	QMap<QString, BaseVersionPtr> m_lookup;

	bool m_loaded = false;
	bool m_hasLocalIndex = false;
	QString m_latestReleaseID = "INVALID";
	QString m_latestSnapshotID = "INVALID";

protected
slots:
	virtual void updateListData(QList<BaseVersionPtr> versions) override;
};
