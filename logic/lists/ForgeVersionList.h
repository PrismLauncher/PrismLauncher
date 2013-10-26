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
#include <QSharedPointer>
#include <QUrl>

#include <QNetworkReply>
#include "BaseVersionList.h"
#include "logic/tasks/Task.h"
#include "logic/net/NetJob.h"

class ForgeVersion;
typedef std::shared_ptr<ForgeVersion> ForgeVersionPtr;

struct ForgeVersion : public BaseVersion
{
	virtual QString descriptor()
	{
		return filename;
	}
	;
	virtual QString name()
	{
		return "Forge " + jobbuildver;
	}
	;
	virtual QString typeString() const
	{
		if (installer_url.isEmpty())
			return "Universal";
		else
			return "Installer";
	}
	;

	int m_buildnr = 0;
	QString universal_url;
	QString changelog_url;
	QString installer_url;
	QString jobbuildver;
	QString mcver;
	QString filename;
};

class ForgeVersionList : public BaseVersionList
{
	Q_OBJECT
public:
	friend class ForgeListLoadTask;

	explicit ForgeVersionList(QObject *parent = 0);

	virtual Task *getLoadTask();
	virtual bool isLoaded();
	virtual const BaseVersionPtr at(int i) const;
	virtual int count() const;
	virtual void sort();

	virtual BaseVersionPtr getLatestStable() const;

	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation,
								int role) const;
	virtual int columnCount(const QModelIndex &parent) const;

protected:
	QList<BaseVersionPtr> m_vlist;

	bool m_loaded;

protected
slots:
	virtual void updateListData(QList<BaseVersionPtr> versions);
};

class ForgeListLoadTask : public Task
{
	Q_OBJECT

public:
	explicit ForgeListLoadTask(ForgeVersionList *vlist);

	virtual void executeTask();

protected
slots:
	void list_downloaded();
	void list_failed();

protected:
	NetJobPtr listJob;
	ForgeVersionList *m_list;
};
