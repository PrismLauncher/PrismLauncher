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
#include <QAbstractListModel>

#include "BaseVersionList.h"
#include "logic/tasks/Task.h"

class JavaListLoadTask;

struct JavaVersion : public BaseVersion
{
	virtual QString descriptor()
	{
		return id;
	}

	virtual QString name()
	{
		return id;
	}

	virtual QString typeString() const
	{
		return arch;
	}

	QString id;
	QString arch;
	QString path;
	bool recommended;
};

typedef std::shared_ptr<JavaVersion> JavaVersionPtr;

class JavaVersionList : public BaseVersionList
{
	Q_OBJECT
public:
	explicit JavaVersionList(QObject *parent = 0);

	virtual Task *getLoadTask();
	virtual bool isLoaded();
	virtual const BaseVersionPtr at(int i) const;
	virtual int count() const;
	virtual void sort();

	virtual BaseVersionPtr getTopRecommended() const;

	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual int columnCount(const QModelIndex &parent) const;

public
slots:
	virtual void updateListData(QList<BaseVersionPtr> versions);

protected:
	QList<BaseVersionPtr> m_vlist;

	bool m_loaded = false;
};

class JavaListLoadTask : public Task
{
	Q_OBJECT

public:
	explicit JavaListLoadTask(JavaVersionList *vlist);
	~JavaListLoadTask();

	virtual void executeTask();

protected:
	JavaVersionList *m_list;
	JavaVersion *m_currentRecommended;
};
