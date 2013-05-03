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

#ifndef INSTVERSIONLIST_H
#define INSTVERSIONLIST_H

#include <QObject>
#include <QVariant>
#include <QAbstractListModel>

#include "libmmc_config.h"

class InstVersion;
class Task;

/*!
 * \brief Class that each instance type's version list derives from. 
 * Version lists are the lists that keep track of the available game versions 
 * for that instance. This list will not be loaded on startup. It will be loaded 
 * when the list's load function is called. Before using the version list, you
 * should check to see if it has been loaded yet and if not, load the list.
 * 
 * Note that this class also inherits from QAbstractListModel. Methods from that
 * class determine how this version list shows up in a list view. Said methods
 * all have a default implementation, but they can be overridden by plugins to
 * change the behavior of the list.
 */
class LIBMULTIMC_EXPORT InstVersionList : public QAbstractListModel
{
	Q_OBJECT
public:
	enum ModelRoles
	{
		VersionPointerRole = 0x34B1CB48
	};
	
	explicit InstVersionList(QObject *parent = 0);
	
	/*!
	 * \brief Gets a task that will reload the version islt.
	 * Simply execute the task to load the list.
	 * The task returned by this function should reset the model when it's done.
	 * \return A pointer to a task that reloads the version list.
	 */
	virtual Task *getLoadTask() = 0;
	
	//! Checks whether or not the list is loaded. If this returns false, the list should be loaded.
	virtual bool isLoaded() = 0;
	
	//! Gets the version at the given index.
	virtual const InstVersion *at(int i) const = 0;
	
	//! Returns the number of versions in the list.
	virtual int count() const = 0;
	
	
	//////// List Model Functions ////////
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual int rowCount(const QModelIndex &parent) const;
	virtual int columnCount(const QModelIndex &parent) const;
	
	
	/*!
	 * \brief Finds a version by its descriptor.
	 * \param The descriptor of the version to find.
	 * \return A const pointer to the version with the given descriptor. NULL if
	 * one doesn't exist.
	 */
	virtual const InstVersion *findVersion(const QString &descriptor);
	
	/*!
	 * \brief Gets the latest stable version of this instance type.
	 * This is the version that will be selected by default.
	 * By default, this is simply the first version in the list.
	 */
	virtual const InstVersion *getLatestStable();
	
protected slots:
	/*!
	 * Updates this list with the given list of versions.
	 * This is done by copying each version in the given list and inserting it
	 * into this one.
	 * We need to do this so that we can set the parents of the versions are set to this
	 * version list. This can't be done in the load task, because the versions the load
	 * task creates are on the load task's thread and Qt won't allow their parents
	 * to be set to something created on another thread.
	 * To get around that problem, we invoke this method on the GUI thread, which
	 * then copies the versions and sets their parents correctly.
	 * \param versions List of versions whose parents should be set.
	 */
	virtual void updateListData(QList<InstVersion *> versions) = 0;
};

#endif // INSTVERSIONLIST_H
