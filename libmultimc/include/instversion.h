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

#ifndef INSTVERSION_H
#define INSTVERSION_H

#include <QObject>

#include "libmmc_config.h"

class InstVersionList;

/*!
 * An abstract base class for instance versions.
 * InstVersions hold information about versions such as their names, identifiers,
 * types, etc.
 */
class LIBMULTIMC_EXPORT InstVersion : public QObject
{
	Q_OBJECT
	
	/*!
	 * A string used to identify this version in config files.
	 * This should be unique within the version list or shenanigans will occur.
	 */
	Q_PROPERTY(QString descriptor READ descriptor CONSTANT)
	
	/*!
	 * The name of this version as it is displayed to the user.
	 * For example: "1.5.1"
	 */
	Q_PROPERTY(QString name READ name)
	
	/*!
	 * The name of this version's type as it is displayed to the user.
	 * For example: "Latest Version", "Snapshot", or "MCNostalgia"
	 */
	Q_PROPERTY(QString typeName READ typeName)
	
	/*!
	 * Gets the version's timestamp.
	 * This is primarily used for sorting versions in a list.
	 */
	Q_PROPERTY(qint64 timestamp READ timestamp)
	
	
public:
	/*!
	 * \brief Constructs a new InstVersion with the given parent. 
	 * The parent *must* be the InstVersionList that contains this InstVersion. 
	 * The InstVersion will be added to the list immediately after being created.
	 */
	explicit InstVersion(const QString &descriptor, 
						 const QString &name,
						 qint64 timestamp,
						 InstVersionList *parent = 0);
	
	/*!
	 * Copy constructor.
	 * If the 'parent' parameter is not NULL, sets this version's parent to the
	 * specified object, rather than setting it to the same parent as the version
	 * we're copying from.
	 * \param other The version to copy.
	 * \param parent If not NULL, will be set as the new version object's parent.
	 */
	InstVersion(const InstVersion &other, QObject *parent = 0);
	
	virtual QString descriptor() const;
	virtual QString name() const;
	virtual QString typeName() const = 0;
	virtual qint64 timestamp() const;
	
	virtual InstVersionList *versionList() const;
	
	/*!
	 * Creates a copy of this version with a different parent.
	 * \param newParent The parent QObject of the copy.
	 * \return A new, identical copy of this version with the given parent set.
	 */
	virtual InstVersion *copyVersion(InstVersionList *newParent) const = 0;
	
	/*!
	 * Checks if this version is less (older) than the given version.
	 * \param other The version to compare this one to.
	 * \return True if this version is older than the given version.
	 */
	virtual bool isLessThan(const InstVersion &other) const;
	
	/*!
	 * Checks if this version is greater (newer) than the given version.
	 * \param other The version to compare this one to.
	 * \return True if this version is newer than the given version.
	 */
	virtual bool isGreaterThan(const InstVersion &other) const;
	
	/*!
	 * \sa shouldSortBefore()
	 */
	virtual bool operator<(const InstVersion &rhs) { return isLessThan(rhs); }
	
	/*!
	 * \sa shouldSortAfter()
	 */
	virtual bool operator>(const InstVersion &rhs) { return isGreaterThan(rhs); }
	
protected:
	QString m_descriptor;
	QString m_name;
	qint64 m_timestamp;
};

#endif // INSTVERSION_H
