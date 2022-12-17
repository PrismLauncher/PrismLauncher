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

#pragma once

#include "BaseVersion.h"

#include <QJsonObject>
#include <QStringList>
#include <QVector>
#include <memory>

#include "minecraft/VersionFile.h"

#include "BaseEntity.h"

#include "JsonFormat.h"

namespace Meta
{

class Version : public QObject, public BaseVersion, public BaseEntity
{
    Q_OBJECT

public:
    using Ptr = std::shared_ptr<Version>;

    explicit Version(const QString &uid, const QString &version);
    virtual ~Version();

    QString descriptor() override;
    QString name() override;
    QString typeString() const override;

    QString uid() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uid;
    }
    QString version() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version;
    }
    QString type() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_type;
    }
    QDateTime time() const;
    qint64 rawTime() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_time;
    }
    const Meta::RequireSet &requires() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_requires;
    }
    VersionFilePtr data() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_data;
    }
    bool isRecommended() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_recommended;
    }
    bool isLoaded() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_data != nullptr;
    }

    void merge(const Version::Ptr &other);
    void mergeFromList(const Version::Ptr &other);
    void parse(const QJsonObject &obj) override;

    QString localFilename() const override;

public: // for usage by format parsers only
    void setType(const QString &type);
    void setTime(const qint64 time);
    void setRequires(const Meta::RequireSet &requires, const Meta::RequireSet &conflicts);
    void setVolatile(bool volatile_);
    void setRecommended(bool recommended);
    void setProvidesRecommendations();
    void setData(const VersionFilePtr &data);

signals:
    void typeChanged();
    void timeChanged();
    void requiresChanged();

private:
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_providesRecommendations = false;
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_recommended = false;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_name;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uid;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_type;
    qint64 hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_time = 0;
    Meta::RequireSet hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_requires;
    Meta::RequireSet hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_conflicts;
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_volatile = false;
    VersionFilePtr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_data;
};
}

Q_DECLARE_METATYPE(Meta::Version::Ptr)
