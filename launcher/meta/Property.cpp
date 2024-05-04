// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 初夏同学 <2411829240@qq.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "Property.h"

#include "Application.h"
#include "JsonFormat.h"

namespace Meta {
Property::Property(QObject* parent) : QObject(parent) {}

Property::Property(const QHash<QString, QString>& properties, QObject* parent) : QObject(parent)
{
    m_properties = properties;
}

void Property::parse(const QJsonObject& obj)
{
    parseProperty(obj, this);
}

void Property::configurate(const std::shared_ptr<Property>& other)
{
    m_properties = other->m_properties;
}

void Property::downloadAndApplyProperties()
{
    if (!isLoaded()) {
        load(Net::Mode::Online);
        NetJob* task = dynamic_cast<NetJob*>(getCurrentTask().get());
        QObject::connect(task, &NetJob::succeeded, [&]() { apply(); });
        QObject::connect(task, &NetJob::failed, [&]() { emit failedApplyProperties(tr("Network problems")); });
    } else {
        apply();
    }
}

inline void Property::apply()
{
    auto s = APPLICATION->settings();
    QHash<QString, QString> succeed;
    for (auto& propertyKey : m_properties.keys())
        if (s->contains(propertyKey)) {
            s->set(propertyKey, m_properties[propertyKey]);
            succeed[propertyKey] = m_properties[propertyKey];
        }
    emit succeededApplyProperties(succeed);
}
}  // namespace Meta
