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

#ifndef MINECRAFTVERSIONLIST_H
#define MINECRAFTVERSIONLIST_H

#include <QObject>

#include <QNetworkAccessManager>

#include <QList>

#include "instversionlist.h"

#include "task.h"

#include "minecraftversion.h"

#include "libmmc_config.h"

class MCVListLoadTask;

class LIBMULTIMC_EXPORT MinecraftVersionList : public InstVersionList
{
	Q_OBJECT
public:
	friend class MCVListLoadTask;
	
	explicit MinecraftVersionList(QObject *parent = 0);
	
	virtual Task *getLoadTask();
	virtual bool isLoaded();
	virtual const InstVersion *at(int i) const;
	virtual int count() const;
	virtual void printToStdOut() const;
	virtual void sort();
	
	virtual InstVersion *getLatestStable() const;
	
	/*!
	 * Gets the main version list instance.
	 */
	static MinecraftVersionList &getMainList();
	
	
protected:
	QList<InstVersion *>m_vlist;
	
	bool m_loaded;
	
protected slots:
	virtual void updateListData(QList<InstVersion *> versions);
};

class MCVListLoadTask : public Task
{
	Q_OBJECT
	
public:
	explicit MCVListLoadTask(MinecraftVersionList *vlist);
	~MCVListLoadTask();
	
	virtual void executeTask();
	
protected:
	void setSubStatus(const QString msg = "");
	
	//! Loads versions from Mojang's official version list.
	bool loadFromVList();
	
	//! Loads versions from assets.minecraft.net. Any duplicates are ignored.
	bool loadFromAssets();
	
	//! Loads versions from MCNostalgia.
	bool loadMCNostalgia();
	
	//! Finalizes loading by updating the version list.
	bool finalize();
	
	void updateStuff();
	
	QNetworkAccessManager *netMgr;
	
	MinecraftVersionList *m_list;
	QList<InstVersion *> tempList; //! < List of loaded versions
	QList<InstVersion *> assetsList; //! < List of versions loaded from assets.minecraft.net
	QList<InstVersion *> mcnList; //! < List of loaded MCNostalgia versions
	
	MinecraftVersion *m_currentStable;
	
	bool processedMCVListReply;
	bool processedAssetsReply;
	bool processedMCNReply;
};


#endif // MINECRAFTVERSIONLIST_H
