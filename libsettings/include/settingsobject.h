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

#ifndef SETTINGSOBJECT_H
#define SETTINGSOBJECT_H

#include <QObject>
#include <QMap>

#include "libsettings_config.h"

class Setting;

/*!
 * \brief The SettingsObject handles communicating settings between the application and a settings file.
 * The class keeps a list of Setting objects. Each Setting object represents one
 * of the application's settings. These Setting objects are registered with 
 * a SettingsObject and can be managed similarly to the way a list works.
 *
 * \author Andrew Okin
 * \date 2/22/2013
 *
 * \sa Setting
 */
class LIBSETTINGS_EXPORT SettingsObject : public QObject
{
	Q_OBJECT
public:
	explicit SettingsObject(QObject *parent = 0);
	
	/*!
	 * \brief Registers the given setting with this SettingsObject and connects the necessary signals.
	 * This will fail if there is already a setting with the same ID as
	 * the one that is being registered.
	 * \note Registering a setting object causes the SettingsObject to take ownership
	 * of the object. This means that setting's parent will be set to the object
	 * it was registered with. Because the object it was registered with has taken
	 * ownership, it becomes responsible for managing that setting object's memory.
	 * \warning Do \b not delete the setting after registering it.
	 * \param setting A pointer to the setting that will be registered.
	 * \return True if successful. False if registry failed.
	 */
	virtual bool registerSetting(Setting *setting);
	
	/*!
	 * \brief Unregisters the given setting from this SettingsObject and disconnects its signals.
	 * \note This does not delete the setting. Furthermore, when the setting is 
	 * unregistered, the SettingsObject drops ownership of the setting. This means
	 * that if you unregister a setting, its parent is set to null and you become
	 * responsible for freeing its memory.
	 * \param setting The setting to unregister.
	 */
	virtual void unregisterSetting(Setting *setting);
	
	
	/*!
	 * \brief Gets the setting with the given ID.
	 * \param id The ID of the setting to get.
	 * \return A pointer to the setting with the given ID. 
	 * Returns null if there is no setting with the given ID.
	 * \sa operator []()
	 */
	virtual Setting *getSetting(const QString &id) const;
	
	/*!
	 * \brief Same as getSetting()
	 * \param id The ID of the setting to get.
	 * \return A pointer to the setting with the given ID. 
	 * \sa getSetting()
	 */
	inline Setting *operator [](const QString &id) { return getSetting(id); }
	
	
	/*!
	 * \brief Gets the value of the setting with the given ID.
	 * \param id The ID of the setting to get.
	 * \return The setting's value as a QVariant.
	 * If no setting with the given ID exists, returns an invalid QVariant.
	 */
	virtual QVariant get(const QString &id) const;
	
	/*!
	 * \brief Sets the value of the setting with the given ID.
	 * If no setting with the given ID exists, returns false and logs to qDebug
	 * \param id The ID of the setting to change.
	 * \param value The new value of the setting.
	 * \return True if successful, false if it failed.
	 */
	virtual bool set(const QString &id, QVariant value);
	
	/*!
	 * \brief Reverts the setting with the given ID to default.
	 * \param id The ID of the setting to reset.
	 */
	virtual void reset(const QString &id) const;
	
	/*!
	 * \brief Gets a QList with pointers to all of the registered settings.
	 * The order of the entries in the list is undefined.
	 * \return A QList with pointers to all registered settings.
	 */
	virtual QList<Setting *> getSettings();
	
	/*!
	 * \brief Checks if this SettingsObject contains a setting with the given ID.
	 * \param id The ID to check for.
	 * \return True if the SettingsObject has a setting with the given ID.
	 */
	virtual bool contains(const QString &id);
	
signals:
	/*!
	 * \brief Signal emitted when one of this SettingsObject object's settings changes.
	 * This is usually just connected directly to each Setting object's 
	 * settingChanged() signals.
	 * \param setting A reference to the Setting object that changed.
	 * \param value The Setting object's new value.
	 */
	void settingChanged(const Setting &setting, QVariant value);
	
	/*!
	 * \brief Signal emitted when one of this SettingsObject object's settings resets.
	 * This is usually just connected directly to each Setting object's 
	 * settingReset() signals.
	 * \param setting A reference to the Setting object that changed.
	 */
	void settingReset(const Setting &setting);
	
protected slots:
	/*!
	 * \brief Changes a setting.
	 * This slot is usually connected to each Setting object's 
	 * settingChanged() signal. The signal is emitted, causing this slot
	 * to update the setting's value in the config file.
	 * \param setting A reference to the Setting object that changed.
	 * \param value The setting's new value.
	 */
	virtual void changeSetting(const Setting &setting, QVariant value) = 0;
	
	/*!
	 * \brief Resets a setting.
	 * This slot is usually connected to each Setting object's 
	 * settingReset() signal. The signal is emitted, causing this slot
	 * to update the setting's value in the config file.
	 * \param setting A reference to the Setting object that changed.
	 */
	virtual void resetSetting(const Setting &setting) = 0;
	
protected:
	/*!
	 * \brief Connects the necessary signals to the given Setting.
	 * \param setting The setting to connect.
	 */
	virtual void connectSignals(const Setting &setting);
	
	/*!
	 * \brief Disconnects signals from the given Setting.
	 * \param setting The setting to disconnect.
	 */
	virtual void disconnectSignals(const Setting &setting);
	
	/*!
	 * \brief Function used by Setting objects to get their values from the SettingsObject.
	 * \param setting The 
	 * \return 
	 */
	virtual QVariant retrieveValue(const Setting &setting) = 0;
	
	friend class Setting;
	
private:
	QMap<QString, Setting *> m_settings;
};

/*!
 * \brief A global settings object.
 */
LIBSETTINGS_EXPORT extern SettingsObject *globalSettings;

#endif // SETTINGSOBJECT_H
