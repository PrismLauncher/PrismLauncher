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

Property::Property(const QVector<QPair<QString, QString>>& properties, QObject* parent) : QObject(parent)
{
    m_properties = properties;
}

void Property::parse(const QJsonObject& obj)
{
    parseProperty(obj, this);
}

void Property::merge(const std::shared_ptr<Property>& other)
{
    m_properties = other->m_properties;
}

void Property::applyProperties()
{
    if (!isLoaded()) {
        load(Net::Mode::Online);
    }

    auto s = APPLICATION->settings();
    for (auto& property : m_properties) {
        if (s->contains(property.first))
            s->set(property.first, property.second);
    }
}
}  // namespace Meta
