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
#include <QDateTime>

#include <settingsobject.h>

#include "inifile.h"
#include "lists/InstVersionList.h"

#include "libmmc_config.h"

class MinecraftProcess;
class GameUpdateTask;
class InstanceList;
class BaseInstancePrivate;

/*!
 * \brief Base class for instances.
 * This class implements many functions that are common between instances and 
 * provides a standard interface for all instances.
 * 
 * To create a new instance type, create a new class inheriting from this class
 * and implement the pure virtual functions.
 */
class LIBMULTIMC_EXPORT BaseInstance : public QObject
{
	Q_OBJECT
protected:
	/// no-touchy!
	BaseInstance(BaseInstancePrivate * d, const QString &rootDir, SettingsObject * settings, QObject *parent = 0);
public:
	/// virtual destructor to make sure the destruction is COMPLETE
	virtual ~BaseInstance() {};
	
	/// The instance's ID. The ID SHALL be determined by MMC internally. The ID IS guaranteed to be unique.
	QString id() const;
	
	/// get the type of this instance
	QString instanceType() const;
	
	/// Path to the instance's root directory.
	QString rootDir() const;
	
	/*!
	 * \brief Gets the instance list that this instance is a part of. 
	 *        Returns NULL if this instance is not in a list 
	 *        (the parent is not an InstanceList).
	 * \return A pointer to the InstanceList containing this instance. 
	 */
	InstanceList *instList() const;
	
	
	//////// INSTANCE INFO ////////
	
	/// The name of the instance that is displayed to the user.
	QString name() const;
	
	/// Set the name of the instance that is displayed to the user.
	void setName(QString val);
	
	/// The instance's icon key.
	QString iconKey() const;
	
	/// Set the instance's icon key.
	void setIconKey(QString val);
	
	//! The instance's notes.
	QString notes() const;
	
	/// set the instance notes text
	void setNotes(QString val);
	
	//! The instance's group.
	QString group() const;
	
	/// set the instance group
	void setGroup(QString val);
	
	//// Timestamps ////
	
	/**
	 * Gets the time that the instance was last launched.
	 * Stored in milliseconds since epoch.
	 */
	qint64 lastLaunch() const;
	/// Sets the last launched time to 'val' milliseconds since epoch
	void setLastLaunch(qint64 val = QDateTime::currentMSecsSinceEpoch());
	
	
	/*!
	 * \brief Gets a pointer to this instance's version list.
	 * \return A pointer to the available version list for this instance.
	 */
	virtual InstVersionList *versionList() const;
	
	/*!
	 * \brief Gets this instance's settings object.
	 * This settings object stores instance-specific settings.
	 * \return A pointer to this instance's settings object.
	 */
	virtual SettingsObject &settings() const;
	
	/// returns a valid update task if update is needed, NULL otherwise
	virtual GameUpdateTask* doUpdate() = 0;
	
	/// returns a valid minecraft process, ready for launch
	virtual MinecraftProcess* prepareForLaunch(QString user, QString session) = 0;
	
signals:
	/*!
	 * \brief Signal emitted when properties relevant to the instance view change
	 */
	void propertiesChanged(BaseInstance * inst);
	
protected:
	QSharedPointer<BaseInstancePrivate> inst_d;
};

// pointer for lazy people
typedef QSharedPointer<BaseInstance> InstancePtr;

