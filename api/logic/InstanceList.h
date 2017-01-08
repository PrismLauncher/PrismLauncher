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
#include <QSet>
#include <QList>

#include "BaseInstance.h"
#include "BaseInstanceProvider.h"

#include "multimc_logic_export.h"

#include "QObjectPtr.h"

class QFileSystemWatcher;
class BaseInstance;
class QDir;

class MULTIMC_LOGIC_EXPORT InstanceList : public QAbstractListModel
{
	Q_OBJECT

public:
	explicit InstanceList(SettingsObjectPtr globalSettings, const QString &instDir, QObject *parent = 0);
	virtual ~InstanceList();

public:
	QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;

	enum AdditionalRoles
	{
		GroupRole = Qt::UserRole,
		InstancePointerRole = 0x34B1CB48, ///< Return pointer to real instance
		InstanceIDRole = 0x34B1CB49 ///< Return id if the instance
	};
	/*!
	 * \brief Error codes returned by functions in the InstanceList class.
	 * NoError Indicates that no error occurred.
	 * UnknownError indicates that an unspecified error occurred.
	 */
	enum InstListError
	{
		NoError = 0,
		UnknownError
	};

	InstancePtr at(int i) const
	{
		return m_instances.at(i);
	}

	int count() const
	{
		return m_instances.count();
	}

	InstListError loadList(bool complete = false);

	/// Add an instance provider. Takes ownership of it. Should only be done before the first load.
	void addInstanceProvider(BaseInstanceProvider * provider);

	InstancePtr getInstanceById(QString id) const;
	QModelIndex getInstanceIndexById(const QString &id) const;
	QStringList getGroups();

	void deleteGroup(const QString & name);

signals:
	void dataIsInvalid();

private slots:
	void propertiesChanged(BaseInstance *inst);
	void groupsPublished(QSet<QString>);
	void providerUpdated();

private:
	int getInstIndex(BaseInstance *inst) const;
	void suspendWatch();
	void resumeWatch();
	void add(const QList<InstancePtr> &list);

protected:
	int m_watchLevel = 0;
	QSet<BaseInstanceProvider *> m_updatedProviders;
	QString m_instDir;
	QList<InstancePtr> m_instances;
	QSet<QString> m_groups;
	SettingsObjectPtr m_globalSettings;
	QVector<shared_qobject_ptr<BaseInstanceProvider>> m_providers;
};
