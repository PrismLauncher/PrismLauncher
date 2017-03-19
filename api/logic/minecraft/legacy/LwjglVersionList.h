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
#include <QAbstractListModel>
#include <QUrl>
#include <QNetworkReply>
#include <memory>

#include "BaseVersion.h"
#include "BaseVersionList.h"

#include "multimc_logic_export.h"
#include <net/NetJob.h>

class LWJGLVersion;
typedef std::shared_ptr<LWJGLVersion> PtrLWJGLVersion;

class MULTIMC_LOGIC_EXPORT LWJGLVersion : public BaseVersion
{
public:
	LWJGLVersion(const QString &name, const QString &url)
		: m_name(name), m_url(url)
	{
	}

	virtual QString descriptor()
	{
		return m_name;
	}

	virtual QString name()
	{
		return m_name;
	}

	virtual QString typeString() const
	{
		return QObject::tr("Upstream");
	}

	QString url() const
	{
		return m_url;
	}

protected:
	QString m_name;
	QString m_url;
};

class MULTIMC_LOGIC_EXPORT LWJGLVersionList : public BaseVersionList
{
	Q_OBJECT
public:
	explicit LWJGLVersionList(QObject *parent = 0);

	bool isLoaded() override
	{
		return m_vlist.length() > 0;
	}
	virtual const BaseVersionPtr at(int i) const override
	{
		return m_vlist[i];
	}

	virtual shared_qobject_ptr<Task> getLoadTask() override
	{
		return m_rssDLJob;
	}

	virtual void sortVersions() override {};

	virtual void updateListData(QList< BaseVersionPtr > versions) override {};

	int count() const override
	{
		return m_vlist.length();
	}

	virtual QVariant data(const QModelIndex &index, int role) const override;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	virtual int rowCount(const QModelIndex &parent) const override
	{
		return count();
	}
	virtual int columnCount(const QModelIndex &parent) const override;

public slots:
	virtual void loadList();

private slots:
	void rssFailed(const QString & reason);
	void rssSucceeded();

private:
	QList<PtrLWJGLVersion> m_vlist;
	Net::Download::Ptr m_rssDL;
	NetJobPtr m_rssDLJob;
	QByteArray m_rssData;
	bool m_loading = false;
};
