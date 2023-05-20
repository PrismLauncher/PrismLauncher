/* Copyright 2013-2021 MultiMC Contributors
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

#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
#include <QObject>
#include <QStringList>
#include <QVariant>
#include <memory>

class Setting;
class SettingsObject;

typedef std::shared_ptr<SettingsObject> SettingsObjectPtr;
typedef std::weak_ptr<SettingsObject> SettingsObjectWeakPtr;

/*!
 * \brief The SettingsObject handles communicating settings between the application and a
 *settings file.
 * The class keeps a list of Setting objects. Each Setting object represents one
 * of the application's settings. These Setting objects are registered with
 * a SettingsObject and can be managed similarly to the way a list works.
 *
 * \author Andrew Okin
 * \date 2/22/2013
 *
 * \sa Setting
 */
class SettingsObject : public QObject {
    Q_OBJECT
   public:
    class Lock {
       public:
        Lock(SettingsObjectPtr locked) : m_locked(locked) { m_locked->suspendSave(); }
        ~Lock() { m_locked->resumeSave(); }

       private:
        SettingsObjectPtr m_locked;
    };

   public:
    explicit SettingsObject(QObject* parent = 0);
    virtual ~SettingsObject();
    /*!
     * Registers an override setting for the given original setting in this settings object
     * gate decides if the passthrough (true) or the original (false) is used for value
     *
     * This will fail if there is already a setting with the same ID as
     * the one that is being registered.
     * \return A valid Setting shared pointer if successful.
     */
    std::shared_ptr<Setting> registerOverride(std::shared_ptr<Setting> original, std::shared_ptr<Setting> gate);

    /*!
     * Registers a passthorugh setting for the given original setting in this settings object
     * gate decides if the passthrough (true) or the original (false) is used for value
     *
     * This will fail if there is already a setting with the same ID as
     * the one that is being registered.
     * \return A valid Setting shared pointer if successful.
     */
    std::shared_ptr<Setting> registerPassthrough(std::shared_ptr<Setting> original, std::shared_ptr<Setting> gate);

    /*!
     * Registers the given setting with this SettingsObject and connects the necessary  signals.
     *
     * This will fail if there is already a setting with the same ID as
     * the one that is being registered.
     * \return A valid Setting shared pointer if successful.
     */
    std::shared_ptr<Setting> registerSetting(QStringList synonyms, QVariant defVal = QVariant());

    /*!
     * Registers the given setting with this SettingsObject and connects the necessary signals.
     *
     * This will fail if there is already a setting with the same ID as
     * the one that is being registered.
     * \return A valid Setting shared pointer if successful.
     */
    std::shared_ptr<Setting> registerSetting(QString id, QVariant defVal = QVariant()) { return registerSetting(QStringList(id), defVal); }

    /*!
     * Un-Registers the given setting with this SettingsObject and disconnects the necessary signals.
     *
     */
    void unregisterSetting(std::shared_ptr<Setting> setting);

    /*!
     * \brief Gets the setting with the given ID.
     * \param id The ID of the setting to get.
     * \return A pointer to the setting with the given ID.
     * Returns null if there is no setting with the given ID.
     * \sa operator []()
     */
    std::shared_ptr<Setting> getSetting(const QString& id) const;

    /*!
     * \brief Gets the value of the setting with the given ID.
     * \param id The ID of the setting to get.
     * \return The setting's value as a QVariant.
     * If no setting with the given ID exists, returns an invalid QVariant.
     */
    QVariant get(const QString& id) const;
    QVariant get(const QStringList& id_parts) { return get(id_parts.join('/')); }

    /*!
     * \brief Sets the value of the setting with the given ID.
     * If no setting with the given ID exists, returns false
     * \param id The ID of the setting to change.
     * \param value The new value of the setting.
     * \return True if successful, false if it failed.
     */
    bool set(const QString& id, QVariant value);
    bool set(const QStringList& id_parts, QVariant value) { return set(id_parts.join('/'), value); }

    /*!
     * \brief Sets the value of the setting with the given ID.
     * If no setting with the given ID exists the setting is created with the id as the only synonym.
     * \param id The ID of the setting to change.
     * \param value The new value of the setting.
     * \return a valid Setting shared pointer
     */
    std::shared_ptr<Setting> setOrRegister(const QString& id, QVariant value);
    std::shared_ptr<Setting> setOrRegister(const QStringList& id_parts, QVariant value) { return setOrRegister(id_parts.join('/'), value); }

    /*!
     * \brief Reverts the setting with the given ID to default.
     * \param id The ID of the setting to reset.
     */
    void reset(const QString& id) const;

    /*!
     * \brief Removes a setting from the backing storage and this container.
     *
     * If id is a leagin path for some existing setting instead of a fully
     * qualified ID removes all settings under that path.
     * \param id The ID of the setting to remove.
     * \return True if successful, false if it failed
     */
    bool remove(const QString& id);
    bool remove(const QStringList& path_parts) { return remove(path_parts.join('/')); }

    /*!
     * \brief Removes all settings that fall under the given path.
     * \param path the leading path of the settings to remove.
     * \return True if successful, false if it failed
     */
    bool removeGroup(const QString& path);

    /*!
     * \brief Checks if this SettingsObject contains a setting with the given ID.
     * \param id The ID to check for.
     * \return True if the SettingsObject has a setting with the given ID.
     */
    bool contains(const QString& id);

    /*!
     * \brief Reloads the settings and emit signals for changed settings
     * \return True if reloading was successful
     */
    virtual bool reload();

    virtual void suspendSave() = 0;
    virtual void resumeSave() = 0;

    /*!
     * \brief Returns a list of all paths that begin with the given path.
     * if the path is empty returns the top level groups
     * \return A list of string setting paths/
     */
    QStringList childGroups(const QString& path = "");
    QStringList childGroups(const QStringList& path_parts) { return childGroups(path_parts.join('/')); }

    /*!
     * \brief Returns a list of all keys that begin with the given path.
     * If the path is empty returns all top level keys.
     * \return A list of string key ids.
     */
    QStringList childKeys(const QString& path = "");
    QStringList childKeys(const QStringList& path_parts) { return childKeys(path_parts.join('/')); }

   signals:
    /*!
     * \brief Signal emitted when one of this SettingsObject object's settings changes.
     * This is usually just connected directly to each Setting object's
     * SettingChanged() signals.
     * \param setting A reference to the Setting object that changed.
     * \param value The Setting object's new value.
     */
    void SettingChanged(const Setting& setting, QVariant value);

    /*!
     * \brief Signal emitted when one of this SettingsObject object's settings resets.
     * This is usually just connected directly to each Setting object's
     * settingReset() signals.
     * \param setting A reference to the Setting object that changed.
     */
    void settingReset(const Setting& setting);

    /*!
     * \brief Signal emitten when one of this SettingsObject object's settings is removed.
     * This is usually just connected directly to each Settings object's
     * SettingRemoved signals.
     * \paral settings A reference to the Setting object that was removed.
     */
    void settingRemoved(const Setting& setting);

   protected slots:
    /*!
     * \brief Changes a setting.
     * This slot is usually connected to each Setting object's
     * SettingChanged() signal. The signal is emitted, causing this slot
     * to update the setting's value in the config file.
     * \param setting A reference to the Setting object that changed.
     * \param value The setting's new value.
     */
    virtual void changeSetting(const Setting& setting, QVariant value) = 0;

    /*!
     * \brief Resets a setting.
     * This slot is usually connected to each Setting object's
     * settingReset() signal. The signal is emitted, causing this slot
     * to update the setting's value in the config file.
     * \param setting A reference to the Setting object that changed.
     */
    virtual void resetSetting(const Setting& setting) = 0;

   protected:
    /*!
     * \brief Connects the necessary signals to the given Setting.
     * \param setting The setting to connect.
     */
    void connectSignals(const Setting& setting);

    /*!
     * \brief Disconnects the necessary signals to the given Setting.
     * \param setting The setting to disconnect.
     */
    void disconnectSignals(const Setting& setting);

    /*!
     * \brief Function used by Setting objects to get their values from the SettingsObject.
     * \param setting The
     * \return
     */
    virtual QVariant retrieveValue(const Setting& setting) = 0;
   
   /*!
    * \brief Function used by Settings object to clear their storage in the SettingsObject.
    * \param setting The settig to clear storage for.
    */ 
    virtual void removeValue(const Setting& setting) = 0;

    friend class Setting;

   private:
    QMap<QString, std::shared_ptr<Setting>> m_settings;

   protected:
    bool m_suspendSave = false;
    bool m_doSave = false;
};
