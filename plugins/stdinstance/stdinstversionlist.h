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

#ifndef STDINSTVERSIONLIST_H
#define STDINSTVERSIONLIST_H

#include <instversionlist.h>

#include <task.h>

class QNetworkReply;

class StdInstVListLoadTask;
class StdInstVersion;

class StdInstVersionList : public InstVersionList
{
	Q_OBJECT
public:
	friend class StdInstVListLoadTask;
	
	explicit StdInstVersionList(QObject *parent = 0);
	
	virtual Task *getLoadTask();
	
	virtual bool isLoaded();
	
	virtual const InstVersion *at(int i) const;
	
	virtual int count() const;
	
	//! Prints the list to stdout. This is mainly for debugging.
	virtual void printToStdOut();
	
protected:
	QList<InstVersion *> m_vlist;
	
	bool loaded;
};

class StdInstVListLoadTask : public Task
{
Q_OBJECT
public:
	StdInstVListLoadTask(StdInstVersionList *vlist);
	
	virtual void executeTask();
	
	//! Performs some final processing when the task is finished.
	virtual void finalize();
	
protected slots:
	//! Slot connected to the finished signal for mcdlReply.
	virtual void processMCDLReply();
	
	//! Slot connected to the finished signal for assetsReply.
	virtual void processAssetsReply();
	
	//! Slot connected to the finished signal for mcnReply.
	virtual void processMCNReply();
	
protected:
	void setSubStatus(const QString &msg = "");
	
	StdInstVersionList *m_list;
	QList<InstVersion *> tempList; //! < List of loaded versions.
	QList<InstVersion *> mcnList; //! < List of MCNostalgia versions. 
	
	QNetworkReply *assetsReply; //! < The reply from assets.minecraft.net
	QNetworkReply *mcdlReply; //! < The reply from s3.amazonaws.com/MinecraftDownload
	QNetworkReply *mcnReply; //! < The reply from MCNostalgia.
	
	bool processedAssetsReply;
	bool processedMCDLReply;
	bool processedMCNReply;
	
	//! Checks if the task is finished processing replies and, if so, exits the task's event loop.
	void updateStuff();
	
	StdInstVersion *currentStable;
	
	bool foundCurrentInAssets;
};

extern StdInstVersionList vList;

#endif // STDINSTVERSIONLIST_H
