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

#include "PassthroughSetting.h"

PassthroughSetting::PassthroughSetting(std::shared_ptr<Setting> other, std::shared_ptr<Setting> gate)
    : Setting(other->configKeys(), QVariant())
{
    Q_ASSERT(other);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_other = other;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_gate = gate;
}

bool PassthroughSetting::isOverriding() const
{
    if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_gate)
    {
        return false;
    }
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_gate->get().toBool();
}

QVariant PassthroughSetting::defValue() const
{
    if(isOverriding())
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_other->get();
    }
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_other->defValue();
}

QVariant PassthroughSetting::get() const
{
    if(isOverriding())
    {
        return Setting::get();
    }
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_other->get();
}

void PassthroughSetting::reset()
{
    if(isOverriding())
    {
        Setting::reset();
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_other->reset();
}

void PassthroughSetting::set(QVariant value)
{
    if(isOverriding())
    {
        Setting::set(value);
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_other->set(value);
}
