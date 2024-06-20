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

#include <QMetaType>
#include <QString>
#include <QVariant>

#include "settings/SettingsFile.h"

#include <toml++/toml.h>

Q_DECLARE_METATYPE(toml::table)
Q_DECLARE_METATYPE(toml::array)
Q_DECLARE_METATYPE(const toml::table*)
Q_DECLARE_METATYPE(const toml::array*)
Q_DECLARE_METATYPE(toml::node*)

class TomlFile : public SettingsFile {
   public:
    explicit TomlFile() = default;

    bool loadFile(QString fileName);
    bool loadFile(QByteArray data);
    bool saveFile(QString fileName);

    QVariant get(QString key, QVariant def = {}) const;
    void set(QString key, QVariant val);
    void remove(QString key);
    bool contains(QString key) const;
    QVariant operator[](const QString& key) const;
    virtual QStringList keys();

   private:
    bool migrate(QString fileName);

   private:
    toml::table m_data;
    bool m_loaded;
};