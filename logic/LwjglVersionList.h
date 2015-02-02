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

#include <memory>
#include "BaseVersion.h"
#include "BaseVersionList.h"

class LWJGLVersion;
typedef std::shared_ptr<LWJGLVersion> PtrLWJGLVersion;

class LWJGLVersion : public BaseVersion
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

class LWJGLVersionList : public BaseVersionList
{
	Q_OBJECT
public:
	explicit LWJGLVersionList(QObject *parent = 0);

	bool isLoaded()
	{
		return m_vlist.length() > 0;
	}
	virtual const BaseVersionPtr at(int i) const override
	{
		return m_vlist[i];
	}

	virtual Task* getLoadTask()
	{
		return nullptr;
	}

	virtual void sort() {};

	virtual void updateListData(QList< BaseVersionPtr > versions) {};

	int count() const
	{
		return m_vlist.length();
	}

	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual int rowCount(const QModelIndex &parent) const
	{
		return count();
	}
	virtual int columnCount(const QModelIndex &parent) const;

	virtual bool isLoading() const;
	virtual bool errored() const
	{
		return m_errored;
	}

	virtual QString lastErrorMsg() const
	{
		return m_lastErrorMsg;
	}

public
slots:
	/*!
	 * Loads the version list.
	 * This is done asynchronously. On success, the loadListFinished() signal will
	 * be emitted. The list model will be reset as well, resulting in the modelReset()
	 * signal being emitted. Note that the model will be reset before loadListFinished() is
	 * emitted.
	 * If loading the list failed, the loadListFailed(QString msg),
	 * signal will be emitted.
	 */
	virtual void loadList();

signals:
	/*!
	 * Emitted when the list either starts or finishes loading.
	 * \param loading Whether or not the list is loading.
	 */
	void loadingStateUpdated(bool loading);

	void loadListFinished();

	void loadListFailed(QString msg);

private:
	QList<PtrLWJGLVersion> m_vlist;

	QNetworkReply *m_netReply;
	QNetworkReply *reply;

	bool m_loading;
	bool m_errored;
	QString m_lastErrorMsg;

	void failed(QString msg);

	void finished();

	void setLoading(bool loading);

private
slots:
	virtual void netRequestComplete();
};
