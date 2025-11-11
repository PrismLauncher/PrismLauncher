// SPDX-FileCopyrightText: 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
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

#include "Resource.h"

#include <QMutex>

class Version;

enum class WorldSaveFormat { SINGLE, MULTI, INVALID };

class WorldSave : public Resource {
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<Resource>;

    WorldSave(QObject* parent = nullptr) : Resource(parent) {}
    WorldSave(QFileInfo file_info) : Resource(file_info) {}

    /** Gets the format of the save. */
    WorldSaveFormat saveFormat() const { return m_save_format; }
    /** Gets the name of the save dir (first found in multi mode). */
    QString saveDirName() const { return m_save_dir_name; }

    /** Thread-safe. */
    void setSaveFormat(WorldSaveFormat new_save_format);
    /** Thread-safe. */
    void setSaveDirName(QString dir_name);

    bool valid() const override;

   protected:
    mutable QMutex m_data_lock;

    /** The format in which the save file is in.
     *  Since saves can be distributed in various slightly different ways, this allows us to treat them separately.
     */
    WorldSaveFormat m_save_format = WorldSaveFormat::INVALID;

    QString m_save_dir_name;
};
