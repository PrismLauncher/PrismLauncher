// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 Trial97 <alexandru.tripon97@gmail.com>
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
 */

#pragma once

#include <QString>
#include <QVariant>

class SettingsFile {
   public:
    virtual ~SettingsFile() = default;
    virtual bool loadFile(QString fileName) = 0;
    virtual bool loadFile(QByteArray data) = 0;
    virtual bool saveFile(QString fileName) = 0;

    virtual QVariant get(QString key, QVariant def = {}) const = 0;
    virtual void set(QString key, QVariant val) = 0;
    virtual void remove(QString key) = 0;
    virtual bool contains(QString key) const = 0;
    virtual QVariant operator[](const QString& key) const = 0;
    virtual QStringList keys() = 0;
};
