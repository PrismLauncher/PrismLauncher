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

#include "settings/SettingsObject.h"
#include "settings/Setting.h"
#include "settings/OverrideSetting.h"
#include "PassthroughSetting.h"
#include <QDebug>

#include <QVariant>
#include <memory>
#include <random>

SettingsObject::SettingsObject(QObject *parent) : QObject(parent)
{
}

SettingsObject::~SettingsObject()
{
    m_settings.clear();
}

std::shared_ptr<Setting> SettingsObject::registerOverride(std::shared_ptr<Setting> original,
                                                          std::shared_ptr<Setting> gate)
{
    if (contains(original->id()))
    {
        qCritical() << QString("Failed to register setting %1. ID already exists.")
                   .arg(original->id());
        return nullptr; // Fail
    }
    auto override = std::make_shared<OverrideSetting>(original, gate);
    override->m_storage = this;
    connectSignals(*override);
    m_settings.insert(override->id(), override);
    return override;
}

std::shared_ptr<Setting> SettingsObject::registerPassthrough(std::shared_ptr<Setting> original,
                                                             std::shared_ptr<Setting> gate)
{
    if (contains(original->id()))
    {
        qCritical() << QString("Failed to register setting %1. ID already exists.")
                   .arg(original->id());
        return nullptr; // Fail
    }
    auto passthrough = std::make_shared<PassthroughSetting>(original, gate);
    passthrough->m_storage = this;
    connectSignals(*passthrough);
    m_settings.insert(passthrough->id(), passthrough);
    return passthrough;
}

std::shared_ptr<Setting> SettingsObject::registerSetting(QStringList synonyms, QVariant defVal)
{
    if (synonyms.empty())
        return nullptr;
    if (contains(synonyms.first()))
    {
        qCritical() << QString("Failed to register setting %1. ID already exists.")
                   .arg(synonyms.first());
        return nullptr; // Fail
    }
    auto setting = std::make_shared<Setting>(synonyms, defVal);
    setting->m_storage = this;
    connectSignals(*setting);
    m_settings.insert(setting->id(), setting);
    return setting;
}

void SettingsObject::unregisterSetting(std::shared_ptr<Setting> setting)
{
    if (!m_settings.contains(setting->id()))
        return;
    disconnectSignals(*setting);
    m_settings.remove(setting->id());
}

std::shared_ptr<Setting> SettingsObject::getSetting(const QString &id) const
{
    // Make sure there is a setting with the given ID.
    if (!m_settings.contains(id))
        return NULL;

    return m_settings[id];
}

QVariant SettingsObject::get(const QString &id) const
{
    auto setting = getSetting(id);
    return (setting ? setting->get() : QVariant());
}

bool SettingsObject::set(const QString &id, QVariant value)
{
    auto setting = getSetting(id);
    if (!setting)
    {
        qCritical() << QString("Error changing setting %1. Setting doesn't exist.").arg(id);
        return false;
    }
    else
    {
        setting->set(value);
        return true;
    }
}

std::shared_ptr<Setting> SettingsObject::setOrRegister(const QString& id, QVariant value)
{
    auto setting = getSetting(id);
    if (!setting)
        setting = registerSetting(id, value);
    setting->set(value);
    return setting;
}

void SettingsObject::reset(const QString &id) const
{
    auto setting = getSetting(id);
    if (setting)
        setting->reset();
}

bool SettingsObject::remove(const QString& id)
{
    auto setting = getSetting(id);
    if (setting) {
        setting->remove();
        unregisterSetting(setting);
        return true;
    } else {
        return removeGroup(id);
    }
}

bool SettingsObject::removeGroup(const QString& path)
{
    // build list of settings to remove (no collection modification in loops over that collection!)
    QList<std::shared_ptr<Setting>> to_remove = {};
    auto path_parts = path.split('/');
    if (!path_parts.isEmpty() && (path_parts.first() == ""))
        path_parts.removeFirst();
    for (auto setting : m_settings) {
        auto id = setting->id();
        auto id_parts = id.split('/');
        bool match = true;
        for (auto part : path_parts) {
            if (match && !id_parts.isEmpty()) {
                auto cur = id_parts.takeFirst();
                match = part == cur;
            } else {
                match = false;
            }
        }
        if (match || path.isEmpty())
            to_remove.append(setting);
    }
    for (auto setting : to_remove) {
        setting->remove();
        unregisterSetting(setting);
    }
    return !to_remove.isEmpty();
}

bool SettingsObject::contains(const QString &id)
{
    return m_settings.contains(id);
}

bool SettingsObject::reload()
{
    for (auto setting : m_settings.values())
    {
        setting->set(setting->get());
    }
    return true;
}

QStringList SettingsObject::childGroups(const QString& path)
{
    QStringList child_groups = {};
    auto path_parts = path.split('/');
    if (!path_parts.isEmpty() && (path_parts.first() == ""))
        path_parts.removeFirst();
    for (auto setting : m_settings) {
        auto id = setting->id();
        auto id_parts = id.split('/');
        bool match = true;
        for (auto part: path_parts) {
            if (match && !id_parts.isEmpty()){
                auto cur = id_parts.takeFirst();
                match = part == cur;
            } else {
                match = false;
            }
        }
        if ((match || path.isEmpty()) && !id_parts.isEmpty()) {
            auto key = id_parts.takeFirst();
            if (!id_parts.isEmpty() && !child_groups.contains(key)) 
                child_groups.append(key); // that was not the last section of the path so this is a group 
        }
    }
    return child_groups;
}

QStringList SettingsObject::childKeys(const QString& path)
{
    QStringList child_keys = {};
    auto path_parts = path.split('/');
    if (!path_parts.isEmpty() && (path_parts.first() == ""))
        path_parts.removeFirst();
    for (auto setting: m_settings) {
        auto id = setting->id();
        auto id_parts = id.split('/');
        bool match = true;
        for (auto part: path_parts) {
            if (match && !id_parts.isEmpty()){
                auto cur = id_parts.takeFirst();
                match = part == cur;
            } else {
                match = false;
            }
        }
        if ((match || path.isEmpty()) && !id_parts.isEmpty()) {
            auto key = id_parts.takeFirst();
            if (id_parts.isEmpty()  && !child_keys.contains(key)) 
                child_keys.append(key); // that was the last section of the path so this is a key 
        }
    }
    return child_keys;
}

void SettingsObject::connectSignals(const Setting &setting)
{
    connect(&setting, &Setting::SettingChanged, this, &SettingsObject::changeSetting);
    connect(&setting, SIGNAL(SettingChanged(const Setting &, QVariant)), this,
            SIGNAL(SettingChanged(const Setting &, QVariant)));

    connect(&setting, &Setting::settingReset, this, &SettingsObject::resetSetting);
    connect(&setting, SIGNAL(settingReset(Setting)), this, SIGNAL(settingReset(const Setting &)));

    connect(&setting, SIGNAL(settingRemoved(Setting)), this, SIGNAL(settingRemoved(const Setting &)));
}

void SettingsObject::disconnectSignals(const Setting& setting)
{
    disconnect(&setting, &Setting::SettingChanged, this, &SettingsObject::changeSetting);
    disconnect(&setting, SIGNAL(SettingChanged(const Setting &, QVariant)), this,
            SIGNAL(SettingChanged(const Setting &, QVariant)));

    disconnect(&setting, &Setting::settingReset, this, &SettingsObject::resetSetting);
    disconnect(&setting, SIGNAL(settingReset(Setting)), this, SIGNAL(settingReset(const Setting &)));

    disconnect(&setting, SIGNAL(settingRemoved(Setting)), this, SIGNAL(settingRemoved(const Setting &)));
}
