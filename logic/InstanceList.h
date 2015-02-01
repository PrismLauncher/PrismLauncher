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
#include <QSet>

#include "logic/BaseInstance.h"

class BaseInstance;
class QDir;

class InstanceList : public QAbstractListModel
{
	Q_OBJECT
private:
	void loadGroupList(QMap<QString, QString> &groupList);

public
slots:
	void saveGroupList();

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

	enum InstLoadError
	{
		NoLoadError = 0,
		UnknownLoadError,
		NotAnInstance
	};

	enum InstCreateError
	{
		NoCreateError = 0,
		NoSuchVersion,
		UnknownCreateError,
		InstExists,
		CantCreateDir
	};

	QString instDir() const
	{
		return m_instDir;
	}

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
	InstancePtr getInstanceById(QString id) const;

	QModelIndex getInstanceIndexById(const QString &id) const;

	// FIXME: instead of iterating through all instances and forming a set, keep the set around
	QStringList getGroups();

	/*!
	 * \brief Creates a stub instance
	 *
	 * \param inst Pointer to store the created instance in.
	 * \param version Game version to use for the instance
	 * \param instDir The new instance's directory.
	 * \return An InstCreateError error code.
	 * - InstExists if the given instance directory is already an instance.
	 * - CantCreateDir if the given instance directory cannot be created.
	 */
	InstCreateError createInstance(InstancePtr &inst, BaseVersionPtr version,
								   const QString &instDir);

	/*!
	 * \brief Creates a copy of an existing instance with a new name
	 *
	 * \param newInstance Pointer to store the created instance in.
	 * \param oldInstance The instance to copy
	 * \param instDir The new instance's directory.
	 * \return An InstCreateError error code.
	 * - InstExists if the given instance directory is already an instance.
	 * - CantCreateDir if the given instance directory cannot be created.
	 */
	InstCreateError copyInstance(InstancePtr &newInstance, InstancePtr &oldInstance,
								 const QString &instDir);

	/*!
	 * \brief Loads an instance from the given directory.
	 * Checks the instance's INI file to figure out what the instance's type is first.
	 * \param inst Pointer to store the loaded instance in.
	 * \param instDir The instance's directory.
	 * \return An InstLoadError error code.
	 * - NotAnInstance if the given instance directory isn't a valid instance.
	 */
	InstLoadError loadInstance(InstancePtr &inst, const QString &instDir);

signals:
	void dataIsInvalid();

public
slots:
	void on_InstFolderChanged(const Setting &setting, QVariant value);

	/*!
	 * \brief Loads the instance list. Triggers notifications.
	 */
	InstListError loadList();

private
slots:
	void propertiesChanged(BaseInstance *inst);
	void instanceNuked(BaseInstance *inst);
	void groupChanged();

private:
	int getInstIndex(BaseInstance *inst) const;

public:
	static bool continueProcessInstance(InstancePtr instPtr, const int error, const QDir &dir,
								 QMap<QString, QString> &groupMap);

protected:
	QString m_instDir;
	QList<InstancePtr> m_instances;
	QSet<QString> m_groups;
	SettingsObjectPtr m_globalSettings;
};
