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
#include <QStringView>
#include <QList>

class QUrl;

class Version
{
public:
    Version(const QString &str);
    Version() {}

    bool operator<(const Version &other) const;
    bool operator<=(const Version &other) const;
    bool operator>(const Version &other) const;
    bool operator>=(const Version &other) const;
    bool operator==(const Version &other) const;
    bool operator!=(const Version &other) const;

    QString toString() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_string;
    }

private:
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_string;
    struct Section
    {
        explicit Section(const QString &fullString)
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fullString = fullString;
            int cutoff = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fullString.size();
            for(int i = 0; i < hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fullString.size(); i++)
            {
                if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fullString[i].isDigit())
                {
                    cutoff = i;
                    break;
                }
            }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            auto numPart = QStringView{hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fullString}.left(cutoff);
#else
            auto numPart = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fullString.leftRef(cutoff);
#endif
            if(numPart.size())
            {
                numValid = true;
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numPart = numPart.toInt();
            }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            auto stringPart = QStringView{hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fullString}.mid(cutoff);
#else
            auto stringPart = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fullString.midRef(cutoff);
#endif
            if(stringPart.size())
            {
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stringPart = stringPart.toString();
            }
        }
        explicit Section() {}
        bool numValid = false;
        int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numPart = 0;
        QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stringPart;
        QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fullString;

        inline bool operator!=(const Section &other) const
        {
            if(numValid && other.numValid)
            {
                return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numPart != other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numPart || hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stringPart != other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stringPart;
            }
            else
            {
                return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fullString != other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fullString;
            }
        }
        inline bool operator<(const Section &other) const
        {
            if(numValid && other.numValid)
            {
                if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numPart < other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numPart)
                    return true;
                if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numPart == other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numPart && hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stringPart < other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stringPart)
                    return true;
                return false;
            }
            else
            {
                return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fullString < other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fullString;
            }
        }
        inline bool operator>(const Section &other) const
        {
            if(numValid && other.numValid)
            {
                if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numPart > other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numPart)
                    return true;
                if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numPart == other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_numPart && hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stringPart > other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stringPart)
                    return true;
                return false;
            }
            else
            {
                return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fullString > other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fullString;
            }
        }
    };
    QList<Section> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sections;

    void parse();
};
