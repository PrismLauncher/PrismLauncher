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

#include <QObject>

#include <QString>
#include <QStringList>
#include "BaseVersion.h"
#include "BaseVersionList.h"
#include "tasks/Task.h"
#include "net/NetJob.h"
#include <minecraft/Library.h>
#include <minecraft/VersionFile.h>

#include "multimc_logic_export.h"

class LLListLoadTask;
class QNetworkReply;

class LiteLoaderVersion : public BaseVersion
{
public:
	QString descriptor() override
	{
		if (isLatest)
		{
			return QObject::tr("Latest");
		}
		return QString();
	}
	QString typeString() const override
	{
		return mcVersion;
	}
	QString name() override
	{
		return version;
	}
	VersionFilePtr getVersionFile();

	// important info
	QString version;
	QString mcVersion;
	QString md5;
	long timestamp = 0;
	bool isLatest = false;
	bool isRecommended = false;
	bool isSnapshot = false;

	QString tweakClass;
	QList<LibraryPtr> libraries;

	// meta
	QString defaultUrl;
	QString description;
	QString authors;
};
typedef std::shared_ptr<LiteLoaderVersion> LiteLoaderVersionPtr;

class MULTIMC_LOGIC_EXPORT LiteLoaderVersionList : public BaseVersionList
{
	Q_OBJECT
public:
	friend class LLListLoadTask;

	explicit LiteLoaderVersionList(QObject *parent = 0);

	Task *getLoadTask() override;
	bool isLoaded() override;
	const BaseVersionPtr at(int i) const override;
	int count() const override;
	void sortVersions() override;
	QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const override;
	RoleList providesRoles() const override;

	virtual BaseVersionPtr getLatestStable() const override;

protected:
	QList<BaseVersionPtr> m_vlist;

	bool m_loaded = false;

protected
slots:
	void updateListData(QList<BaseVersionPtr> versions) override;
};

class LLListLoadTask : public Task
{
	Q_OBJECT

public:
	explicit LLListLoadTask(LiteLoaderVersionList *vlist);
	~LLListLoadTask();

	virtual void executeTask();

protected
slots:
	void listDownloaded();
	void listFailed();

protected:
	NetJobPtr listJob;
	Net::Download::Ptr listDownload;
	LiteLoaderVersionList *m_list;
};

Q_DECLARE_METATYPE(LiteLoaderVersionPtr)
