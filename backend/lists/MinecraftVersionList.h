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

#include <QObject>
#include <QList>
#include <QSet>
#include <QSharedPointer>

#include "InstVersionList.h"
#include "tasks/Task.h"
#include "MinecraftVersion.h"
#include "libmmc_config.h"

class MCVListLoadTask;
class QNetworkReply;

class LIBMULTIMC_EXPORT MinecraftVersionList : public InstVersionList
{
	Q_OBJECT
public:
	friend class MCVListLoadTask;
	
	explicit MinecraftVersionList(QObject *parent = 0);
	
	virtual Task *getLoadTask();
	virtual bool isLoaded();
	virtual const InstVersionPtr at(int i) const;
	virtual int count() const;
	virtual void sort();
	
	virtual InstVersionPtr getLatestStable() const;
	
	/*!
	 * Gets the main version list instance.
	 */
	static MinecraftVersionList &getMainList();
	
	
protected:
	QList<InstVersionPtr > m_vlist;
	
	bool m_loaded;
	
protected slots:
	virtual void updateListData(QList<InstVersionPtr > versions);
};

class MCVListLoadTask : public Task
{
	Q_OBJECT
	
public:
	explicit MCVListLoadTask(MinecraftVersionList *vlist);
	~MCVListLoadTask();
	
	virtual void executeTask();
	
protected slots:
	void list_downloaded();
	
protected:
	//! Loads versions from Mojang's official version list.
	bool loadFromVList();
	
	QNetworkReply *vlistReply;
	MinecraftVersionList *m_list;
	MinecraftVersion *m_currentStable;
	QSet<QString> legacyWhitelist;
};

