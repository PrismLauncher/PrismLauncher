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

#include "Version.h"

#include <QDateTime>

#include "JsonFormat.h"
#include "minecraft/PackProfile.h"

Meta::Version::Version(const QString &uid, const QString &version)
    : BaseVersion(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uid(uid), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version(version)
{
}

Meta::Version::~Version()
{
}

QString Meta::Version::descriptor()
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version;
}
QString Meta::Version::name()
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_data)
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_data->name;
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uid;
}
QString Meta::Version::typeString() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_type;
}

QDateTime Meta::Version::time() const
{
    return QDateTime::fromMSecsSinceEpoch(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_time * 1000, Qt::UTC);
}

void Meta::Version::parse(const QJsonObject& obj)
{
    parseVersion(obj, this);
}

void Meta::Version::mergeFromList(const Meta::Version::Ptr& other)
{
    if(other->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_providesRecommendations)
    {
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_recommended != other->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_recommended)
        {
            setRecommended(other->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_recommended);
        }
    }
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_type != other->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_type)
    {
        setType(other->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_type);
    }
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_time != other->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_time)
    {
        setTime(other->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_time);
    }
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_requires != other->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_requires)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_requires = other->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_requires;
    }
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_conflicts != other->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_conflicts)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_conflicts = other->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_conflicts;
    }
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_volatile != other->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_volatile)
    {
        setVolatile(other->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_volatile);
    }
}

void Meta::Version::merge(const Version::Ptr &other)
{
    mergeFromList(other);
    if(other->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_data)
    {
        setData(other->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_data);
    }
}

QString Meta::Version::localFilename() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uid + '/' + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version + ".json";
}

void Meta::Version::setType(const QString &type)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_type = type;
    emit typeChanged();
}

void Meta::Version::setTime(const qint64 time)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_time = time;
    emit timeChanged();
}

void Meta::Version::setRequires(const Meta::RequireSet &requires, const Meta::RequireSet &conflicts)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_requires = requires;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_conflicts = conflicts;
    emit requiresChanged();
}

void Meta::Version::setVolatile(bool volatile_)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_volatile = volatile_;
}


void Meta::Version::setData(const VersionFilePtr &data)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_data = data;
}

void Meta::Version::setProvidesRecommendations()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_providesRecommendations = true;
}

void Meta::Version::setRecommended(bool recommended)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_recommended = recommended;
}
