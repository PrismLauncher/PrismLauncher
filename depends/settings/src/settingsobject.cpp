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

#include "include/settingsobject.h"
#include "include/setting.h"

#include <QVariant>

SettingsObject::SettingsObject(QObject *parent) : QObject(parent)
{
}

bool SettingsObject::registerSetting(Setting *setting)
{
	// Check if setting is null or we already have a setting with the same ID.
	if (!setting)
	{
		qDebug(QString("Failed to register setting. Setting is null.")
				   .arg(setting->id())
				   .toUtf8());
		return false; // Fail
	}

	if (contains(setting->id()))
	{
		qDebug(QString("Failed to register setting %1. ID already exists.")
				   .arg(setting->id())
				   .toUtf8());
		return false; // Fail
	}

	m_settings.insert(setting->id(), setting);
	setting->setParent(this); // Take ownership.

	// Connect signals.
	connectSignals(*setting);

	// qDebug(QString("Registered setting %1.").arg(setting->id()).toUtf8());
	return true;
}

void SettingsObject::unregisterSetting(Setting *setting)
{
	if (!setting || !m_settings.contains(setting->id()))
		return; // We can't unregister something that's not registered.

	m_settings.remove(setting->id());

	// Disconnect signals.
	disconnectSignals(*setting);

	setting->setParent(NULL); // Drop ownership.
}

Setting *SettingsObject::getSetting(const QString &id) const
{
	// Make sure there is a setting with the given ID.
	if (!m_settings.contains(id))
		return NULL;

	return m_settings[id];
}

QVariant SettingsObject::get(const QString &id) const
{
	Setting *setting = getSetting(id);
	return (setting ? setting->get() : QVariant());
}

bool SettingsObject::set(const QString &id, QVariant value)
{
	Setting *setting = getSetting(id);
	if (!setting)
	{
		qDebug(QString("Error changing setting %1. Setting doesn't exist.").arg(id).toUtf8());
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
	Setting *setting = getSetting(id);
	if (setting)
		setting->reset();
}

QList<Setting *> SettingsObject::getSettings()
{
	return m_settings.values();
}

bool SettingsObject::contains(const QString &id)
{
	return m_settings.contains(id);
}

void SettingsObject::connectSignals(const Setting &setting)
{
	connect(&setting, SIGNAL(settingChanged(const Setting &, QVariant)),
			SLOT(changeSetting(const Setting &, QVariant)));
	connect(&setting, SIGNAL(settingChanged(const Setting &, QVariant)),
			SIGNAL(settingChanged(const Setting &, QVariant)));

	connect(&setting, SIGNAL(settingReset(Setting)), SLOT(resetSetting(const Setting &)));
	connect(&setting, SIGNAL(settingReset(Setting)), SIGNAL(settingReset(const Setting &)));
}

void SettingsObject::disconnectSignals(const Setting &setting)
{
	setting.disconnect(SIGNAL(settingChanged(const Setting &, QVariant)), this,
					   SLOT(changeSetting(const Setting &, QVariant)));
	setting.disconnect(SIGNAL(settingChanged(const Setting &, QVariant)), this,
					   SIGNAL(settingChanged(const Setting &, QVariant)));

	setting.disconnect(SIGNAL(settingReset(const Setting &, QVariant)), this,
					   SLOT(resetSetting(const Setting &, QVariant)));
	setting.disconnect(SIGNAL(settingReset(const Setting &, QVariant)), this,
					   SIGNAL(settingReset(const Setting &, QVariant)));
}
