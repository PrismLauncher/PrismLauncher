// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#pragma once

#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include "DefaultVariable.h"

struct GradleSpecifier
{
    GradleSpecifier()
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_valid = false;
    }
    GradleSpecifier(QString value)
    {
        operator=(value);
    }
    GradleSpecifier & operator =(const QString & value)
    {
        /*
        org.gradle.test.classifiers : service : 1.0 : jdk15 @ jar
         0 "org.gradle.test.classifiers:service:1.0:jdk15@jar"
         1 "org.gradle.test.classifiers"
         2 "service"
         3 "1.0"
         4 "jdk15"
         5 "jar"
        */
        QRegularExpression matcher(QRegularExpression::anchoredPattern("([^:@]+):([^:@]+):([^:@]+)" "(?::([^:@]+))?" "(?:@([^:@]+))?"));
        QRegularExpressionMatch match = matcher.match(value);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_valid = match.hasMatch();
        if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_valid) {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_invalidValue = value;
            return *this;
        }
        auto elements = match.captured();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_groupId = match.captured(1);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_artifactId = match.captured(2);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version = match.captured(3);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_classifier = match.captured(4);
        if(match.lastCapturedIndex() >= 5)
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extension = match.captured(5);
        }
        return *this;
    }
    QString serialize() const
    {
        if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_valid) {
            return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_invalidValue;
        }
        QString retval = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_groupId + ":" + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_artifactId + ":" + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version;
        if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_classifier.isEmpty())
        {
            retval += ":" + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_classifier;
        }
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extension.isExplicit())
        {
            retval += "@" + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extension;
        }
        return retval;
    }
    QString getFileName() const
    {
        if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_valid) {
            return QString();
        }
        QString filename = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_artifactId + '-' + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version;
        if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_classifier.isEmpty())
        {
            filename += "-" + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_classifier;
        }
        filename += "." + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extension;
        return filename;
    }
    QString toPath(const QString & filenameOverride = QString()) const
    {
        if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_valid) {
            return QString();
        }
        QString filename;
        if(filenameOverride.isEmpty())
        {
            filename = getFileName();
        }
        else
        {
            filename = filenameOverride;
        }
        QString path = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_groupId;
        path.replace('.', '/');
        path += '/' + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_artifactId + '/' + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version + '/' + filename;
        return path;
    }
    inline bool valid() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_valid;
    }
    inline QString version() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version;
    }
    inline QString groupId() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_groupId;
    }
    inline QString artifactId() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_artifactId;
    }
    inline void setClassifier(const QString & classifier)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_classifier = classifier;
    }
    inline QString classifier() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_classifier;
    }
    inline QString extension() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extension;
    }
    inline QString artifactPrefix() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_groupId + ":" + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_artifactId;
    }
    bool matchName(const GradleSpecifier & other) const
    {
        return other.artifactId() == artifactId() && other.groupId() == groupId() && other.classifier() == classifier();
    }
    bool operator==(const GradleSpecifier & other) const
    {
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_groupId != other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_groupId)
            return false;
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_artifactId != other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_artifactId)
            return false;
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version != other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version)
            return false;
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_classifier != other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_classifier)
            return false;
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extension != other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extension)
            return false;
        return true;
    }
private:
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_invalidValue;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_groupId;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_artifactId;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_classifier;
    DefaultVariable<QString> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extension = DefaultVariable<QString>("jar");
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_valid = false;
};
