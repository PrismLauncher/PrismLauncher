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

#include "siglist.h"

#include "libinstance_config.h"

class Instance;

class LIBMMCINST_EXPORT InstanceList : public QObject, public SigList<QSharedPointer<Instance>>
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
	 * \brief Loads the instance list.
	 */
	InstListError loadList();
	
	DEFINE_SIGLIST_SIGNALS(QSharedPointer<Instance>);
	SETUP_SIGLIST_SIGNALS(QSharedPointer<Instance>);
protected:
	QString m_instDir;
};

#endif // INSTANCELIST_H
