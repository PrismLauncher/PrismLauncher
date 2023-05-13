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

void SettingsObject::reset(const QString &id) const
{
    auto setting = getSetting(id);
    if (setting)
        setting->reset();
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

void SettingsObject::connectSignals(const Setting &setting)
{
    connect(&setting, &Setting::SettingChanged, this, &SettingsObject::changeSetting);
    connect(&setting, SIGNAL(SettingChanged(const Setting &, QVariant)), this,
            SIGNAL(SettingChanged(const Setting &, QVariant)));

    connect(&setting, &Setting::settingReset, this, &SettingsObject::resetSetting);
    connect(&setting, SIGNAL(settingReset(Setting)), this, SIGNAL(settingReset(const Setting &)));
}
