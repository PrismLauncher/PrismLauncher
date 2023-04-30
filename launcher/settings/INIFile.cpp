// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 flowln <flowlnlnln@gmail.com>
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

#include <QSettings>

INIFile::INIFile()
{
}

bool INIFile::saveFile(QString fileName)
{
    QSettings _settings_obj{ fileName, QSettings::Format::IniFormat };
    _settings_obj.setFallbacksEnabled(false);

    for (Iterator iter = begin(); iter != end(); iter++)
        _settings_obj.setValue(iter.key(), iter.value());

    _settings_obj.sync();

    if (auto status = _settings_obj.status(); status != QSettings::Status::NoError) {
        // Shouldn't be possible!
        Q_ASSERT(status != QSettings::Status::FormatError);

        if (status == QSettings::Status::AccessError)
            qCritical() << "An access error occurred (e.g. trying to write to a read-only file).";

        return false;
    }

    return true;
}

bool INIFile::loadFile(QString fileName)
{
    QSettings _settings_obj{ fileName, QSettings::Format::IniFormat };
    _settings_obj.setFallbacksEnabled(false);

    if (auto status = _settings_obj.status(); status != QSettings::Status::NoError) {
        if (status == QSettings::Status::AccessError)
            qCritical() << "An access error occurred (e.g. trying to write to a read-only file).";
        if (status == QSettings::Status::FormatError)
            qCritical() << "A format error occurred (e.g. loading a malformed INI file).";
        return false;
    }

    for (auto&& key : _settings_obj.allKeys())
        insert(key, _settings_obj.value(key));
 
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

