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
#include <QAbstractListModel>
#include <QUrl>
#include <QNetworkReply>

#include "BaseVersionList.h"
#include "tasks/Task.h"
#include "net/NetJob.h"
#include "forge/ForgeVersion.h"

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

	ForgeVersionPtr findVersionByVersionNr(QString version);

	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual QList<ModelRoles> providesRoles() override;

	virtual int columnCount(const QModelIndex &parent) const;

protected:
	QList<BaseVersionPtr> m_vlist;

	bool m_loaded = false;

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
	virtual void abort();

protected
slots:
	void listDownloaded();
	void listFailed();
	void gradleListFailed();

protected:
	NetJobPtr listJob;
	ForgeVersionList *m_list;

	CacheDownloadPtr listDownload;
	CacheDownloadPtr gradleListDownload;

private:
	bool parseForgeList(QList<BaseVersionPtr> &out);
	bool parseForgeGradleList(QList<BaseVersionPtr> &out);
};
