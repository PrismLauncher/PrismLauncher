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
        return m_string;
    }

private:
    QString m_string;
    struct Section
    {
        explicit Section(const QString &fullString)
        {
            m_fullString = fullString;
            int cutoff = m_fullString.size();
            for(int i = 0; i < m_fullString.size(); i++)
            {
                if(!m_fullString[i].isDigit())
                {
                    cutoff = i;
                    break;
                }
            }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            auto numPart = QStringView{m_fullString}.left(cutoff);
#else
            auto numPart = m_fullString.leftRef(cutoff);
#endif
            if(numPart.size())
            {
                numValid = true;
                m_numPart = numPart.toInt();
            }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            auto stringPart = QStringView{m_fullString}.mid(cutoff);
#else
            auto stringPart = m_fullString.midRef(cutoff);
#endif
            if(stringPart.size())
            {
                m_stringPart = stringPart.toString();
            }
        }
        explicit Section() {}
        bool numValid = false;
        int m_numPart = 0;
        QString m_stringPart;
        QString m_fullString;

        inline bool operator!=(const Section &other) const
        {
            if(numValid && other.numValid)
            {
                return m_numPart != other.m_numPart || m_stringPart != other.m_stringPart;
            }
            else
            {
                return m_fullString != other.m_fullString;
            }
        }
        inline bool operator<(const Section &other) const
        {
            if(numValid && other.numValid)
            {
                if(m_numPart < other.m_numPart)
                    return true;
                if(m_numPart == other.m_numPart && m_stringPart < other.m_stringPart)
                    return true;
                return false;
            }
            else
            {
                return m_fullString < other.m_fullString;
            }
        }
        inline bool operator>(const Section &other) const
        {
            if(numValid && other.numValid)
            {
                if(m_numPart > other.m_numPart)
                    return true;
                if(m_numPart == other.m_numPart && m_stringPart > other.m_stringPart)
                    return true;
                return false;
            }
            else
            {
                return m_fullString > other.m_fullString;
            }
        }
    };
    QList<Section> m_sections;

    void parse();
};
