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

#include "settings/INIFile.h"
#include <FileSystem.h>

#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QSaveFile>
#include <QDebug>
#include "launcherlog.h"

INIFile::INIFile()
{
}

QString INIFile::unescape(QString orig)
{
    QString out;
    QChar prev = QChar::Null;
    for(auto c: orig)
    {
        if(prev == '\\')
        {
            if(c == 'n')
                out += '\n';
            else if(c == 't')
                out += '\t';
            else if(c == '#')
                out += '#';
            else
                out += c;
            prev = QChar::Null;
        }
        else
        {
            if(c == '\\')
            {
                prev = c;
                continue;
            }
            out += c;
            prev = QChar::Null;
        }
    }
    return out;
}

QString INIFile::escape(QString orig)
{
    QString out;
    for(auto c: orig)
    {
        if(c == '\n')
            out += "\\n";
        else if (c == '\t')
            out += "\\t";
        else if(c == '\\')
            out += "\\\\";
        else if(c == '#')
            out += "\\#";
        else
            out += c;
    }
    return out;
}

bool INIFile::saveFile(QString fileName)
{
    QByteArray outArray;
    for (Iterator iter = begin(); iter != end(); iter++)
    {
        QString value = iter.value().toString();
        value = escape(value);
        outArray.append(iter.key().toUtf8());
        outArray.append('=');
        outArray.append(value.toUtf8());
        outArray.append('\n');
    }

    try
    {
        FS::write(fileName, outArray);
    }
    catch (const Exception &e)
    {
        qCCritical(LAUNCHER_LOG) << e.what();
        return false;
    }

    return true;
}


bool INIFile::loadFile(QString fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    bool success = loadFile(file.readAll());
    file.close();
    return success;
}

bool INIFile::loadFile(QByteArray file)
{
    QTextStream in(file);
#if QT_VERSION <= QT_VERSION_CHECK(6, 0, 0)
    in.setCodec("UTF-8");
#endif

    QStringList lines = in.readAll().split('\n');
    for (int i = 0; i < lines.count(); i++)
    {
        QString &lineRaw = lines[i];
        // Ignore comments.
        int commentIndex = 0;
        QString line = lineRaw;
        // Search for comments until no more escaped # are available
        while((commentIndex = line.indexOf('#', commentIndex + 1)) != -1) {
            if(commentIndex > 0 && line.at(commentIndex - 1) == '\\') {
                continue;
            }
            line = line.left(lineRaw.indexOf('#')).trimmed();
        }

        int eqPos = line.indexOf('=');
        if (eqPos == -1)
            continue;
        QString key = line.left(eqPos).trimmed();
        QString valueStr = line.right(line.length() - eqPos - 1).trimmed();

        valueStr = unescape(valueStr);

        QVariant value(valueStr);
        this->operator[](key) = value;
    }

    return true;
}

QVariant INIFile::get(QString key, QVariant def) const
{
    if (!this->contains(key))
        return def;
    else
        return this->operator[](key);
}

void INIFile::set(QString key, QVariant val)
{
    this->operator[](key) = val;
}
