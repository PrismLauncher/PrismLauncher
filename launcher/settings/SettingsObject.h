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

#include <QObject>
#include <QMap>
#include <QStringList>
#include <QVariant>
#include <QJsonDocument>
#include <QJsonArray>
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
class SettingsObject : public QObject
{
    Q_OBJECT
public:
    class Lock
    {
    public:
        Lock(SettingsObjectPtr locked)
            :m_locked(locked)
        {
            m_locked->suspendSave();
        }
        ~Lock()
        {
            m_locked->resumeSave();
        }
    private:
        SettingsObjectPtr m_locked;
    };
public:
    explicit SettingsObject(QObject *parent = 0);
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
    std::shared_ptr<Setting> registerSetting(QStringList synonyms,
                                             QVariant defVal = QVariant());

    /*!
     * Registers the given setting with this SettingsObject and connects the necessary signals.
     *
     * This will fail if there is already a setting with the same ID as
     * the one that is being registered.
     * \return A valid Setting shared pointer if successful.
     */
    std::shared_ptr<Setting> registerSetting(QString id, QVariant defVal = QVariant())
    {
        return registerSetting(QStringList(id), defVal);
    }

    /*!
     * \brief Gets the setting with the given ID.
     * \param id The ID of the setting to get.
     * \return A pointer to the setting with the given ID.
     * Returns null if there is no setting with the given ID.
     * \sa operator []()
     */
    std::shared_ptr<Setting> getSetting(const QString &id) const;

    /*!
     * \brief Gets the value of the setting with the given ID.
     * \param id The ID of the setting to get.
     * \return The setting's value as a QVariant.
     * If no setting with the given ID exists, returns an invalid QVariant.
     */
    QVariant get(const QString &id) const;

    /*!
     * \brief Sets the value of the setting with the given ID.
     * If no setting with the given ID exists, returns false
     * \param id The ID of the setting to change.
     * \param value The new value of the setting.
     * \return True if successful, false if it failed.
     */
    bool set(const QString &id, QVariant value);

    /*!
     * \brief Reverts the setting with the given ID to default.
     * \param id The ID of the setting to reset.
     */
    void reset(const QString &id) const;

    /*!
     * \brief Checks if this SettingsObject contains a setting with the given ID.
     * \param id The ID to check for.
     * \return True if the SettingsObject has a setting with the given ID.
     */
    bool contains(const QString &id);

    /*!
     * \brief Reloads the settings and emit signals for changed settings
     * \return True if reloading was successful
     */
    virtual bool reload();

    virtual void suspendSave() = 0;
    virtual void resumeSave() = 0;
signals:
    /*!
     * \brief Signal emitted when one of this SettingsObject object's settings changes.
     * This is usually just connected directly to each Setting object's
     * SettingChanged() signals.
     * \param setting A reference to the Setting object that changed.
     * \param value The Setting object's new value.
     */
    void SettingChanged(const Setting &setting, QVariant value);

    /*!
     * \brief Signal emitted when one of this SettingsObject object's settings resets.
     * This is usually just connected directly to each Setting object's
     * settingReset() signals.
     * \param setting A reference to the Setting object that changed.
     */
    void settingReset(const Setting &setting);

protected
slots:
    /*!
     * \brief Changes a setting.
     * This slot is usually connected to each Setting object's
     * SettingChanged() signal. The signal is emitted, causing this slot
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
    void connectSignals(const Setting &setting);

    /*!
     * \brief Function used by Setting objects to get their values from the SettingsObject.
     * \param setting The
     * \return
     */
    virtual QVariant retrieveValue(const Setting &setting) = 0;

    friend class Setting;

private:
    QMap<QString, std::shared_ptr<Setting>> m_settings;
protected:
    bool m_suspendSave = false;
    bool m_doSave = false;
};
