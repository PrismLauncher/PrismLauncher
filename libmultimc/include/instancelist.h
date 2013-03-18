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

#ifndef INSTANCELIST_H
#define INSTANCELIST_H

#include <QObject>
#include <QSharedPointer>

#include "instance.h"
#include "libmmc_config.h"

class Instance;

class LIBMULTIMC_EXPORT InstanceList : public QObject
{
	Q_OBJECT
public:
	explicit InstanceList(const QString &instDir, QObject *parent = 0);
	
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
	
	QString instDir() const { return m_instDir; }
	
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
	};
	
	/*!
	 * \brief Get the count of loaded instances
	 */
	int count() const
	{
		return m_instances.count();
	};
	
	/// Clear all instances. Triggers notifications.
	void clear();
	
	/// Add an instance. Triggers notifications, returns the new index
	int add(InstancePtr t);
	
	/// Get an instance by ID
	InstancePtr getInstanceById (QString id);

signals:
	void instanceAdded(int index);
	void instanceChanged(int index);
	void invalidated();
	
protected:
	QString m_instDir;
	QList< InstancePtr > m_instances;
};

#endif // INSTANCELIST_H
