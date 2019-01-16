/* Copyright 2013-2019 MultiMC Contributors
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

#include "OverrideSetting.h"

OverrideSetting::OverrideSetting(std::shared_ptr<Setting> other, std::shared_ptr<Setting> gate)
    : Setting(other->configKeys(), QVariant())
{
    Q_ASSERT(other);
    Q_ASSERT(gate);
    m_other = other;
    m_gate = gate;
}

bool OverrideSetting::isOverriding() const
{
    return m_gate->get().toBool();
}

QVariant OverrideSetting::defValue() const
{
    return m_other->get();
}

QVariant OverrideSetting::get() const
{
    if(isOverriding())
    {
        return Setting::get();
    }
    return m_other->get();
}

void OverrideSetting::reset()
{
    Setting::reset();
}

void OverrideSetting::set(QVariant value)
{
    Setting::set(value);
}
