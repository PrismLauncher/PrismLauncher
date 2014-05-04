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

#include <QObject>
#include <QList>
#include <QSet>

#include "BaseVersionList.h"
#include "logic/tasks/Task.h"
#include "logic/MinecraftVersion.h"

class MCVListLoadTask;
class QNetworkReply;

class MinecraftVersionList : public BaseVersionList
{
	Q_OBJECT
private:
	void sortInternal();
public:
	friend class MCVListLoadTask;

	explicit MinecraftVersionList(QObject *parent = 0);

	virtual Task *getLoadTask();
	virtual bool isLoaded();
	virtual const BaseVersionPtr at(int i) const;
	virtual int count() const;
	virtual void sort();

	virtual BaseVersionPtr getLatestStable() const;

protected:
	QList<BaseVersionPtr> m_vlist;

	bool m_loaded = false;

protected
slots:
	virtual void updateListData(QList<BaseVersionPtr> versions);
};

class MCVListLoadTask : public Task
{
	Q_OBJECT

public:
	explicit MCVListLoadTask(MinecraftVersionList *vlist);
	~MCVListLoadTask();

	virtual void executeTask();

protected
slots:
	void list_downloaded();

protected:
	QNetworkReply *vlistReply;
	MinecraftVersionList *m_list;
	MinecraftVersion *m_currentStable;
};
