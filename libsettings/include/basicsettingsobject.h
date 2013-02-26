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

#ifndef BASICSETTINGSOBJECT_H
#define BASICSETTINGSOBJECT_H

#include <QObject>
#include <QSettings>

#include "settingsobject.h"

#include "libsettings_config.h"

/*!
 * \brief A settings object that stores its settings in a QSettings object.
 */
class LIBSETTINGS_EXPORT BasicSettingsObject : public SettingsObject
{
	Q_OBJECT
public:
	explicit BasicSettingsObject(QObject *parent = 0);
	
protected slots:
	virtual void changeSetting(const Setting &setting, QVariant value);
	
protected:
	virtual QVariant retrieveValue(const Setting &setting);
	
	QSettings config;
};

#endif // BASICSETTINGSOBJECT_H
