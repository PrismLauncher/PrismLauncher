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
#include "categorizedsortfilterproxymodel.h"
#include <QIcon>

#include "logic/BaseInstance.h"

class BaseInstance;

class InstanceList : public QAbstractListModel
{
	Q_OBJECT
private:
	void loadGroupList(QMap<QString, QString> &groupList);
	void saveGroupList();

public:
	explicit InstanceList(const QString &instDir, QObject *parent = 0);
	virtual ~InstanceList();

public:
	QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;

	enum AdditionalRoles
	{
		InstancePointerRole = 0x34B1CB48 ///< Return pointer to real instance
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

	QString instDir() const
	{
		return m_instDir;
	}

	/*!
	 * \brief Loads the instance list. Triggers notifications.
	 */
	InstListError loadList();

	/*!
	 * \brief Get the instance at index
	 */
	InstancePtr at(int i) const
	{
		return m_instances.at(i);
	}
	;

	/*!
	 * \brief Get the count of loaded instances
	 */
	int count() const
	{
		return m_instances.count();
	}
	;

	/// Clear all instances. Triggers notifications.
	void clear();

	/// Add an instance. Triggers notifications, returns the new index
	int add(InstancePtr t);

	/// Get an instance by ID
	InstancePtr getInstanceById(QString id);
signals:
	void dataIsInvalid();

public
slots:
	void on_InstFolderChanged(const Setting &setting, QVariant value);

private
slots:
	void propertiesChanged(BaseInstance *inst);
	void instanceNuked(BaseInstance *inst);
	void groupChanged();

private:
	int getInstIndex(BaseInstance *inst);

protected:
	QString m_instDir;
	QList<InstancePtr> m_instances;
};

class InstanceProxyModel : public KCategorizedSortFilterProxyModel
{
public:
	explicit InstanceProxyModel(QObject *parent = 0);

protected:
	virtual bool subSortLessThan(const QModelIndex &left, const QModelIndex &right) const;
};
