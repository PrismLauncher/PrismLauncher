/* Copyright 2015-2021 MultiMC Contributors
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
