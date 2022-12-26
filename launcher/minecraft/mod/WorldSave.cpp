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

#include "WorldSave.h"

#include "minecraft/mod/tasks/LocalWorldSaveParseTask.h"

void WorldSave::setSaveFormat(WorldSaveFormat new_save_format)
{
    QMutexLocker locker(&m_data_lock);

    m_save_format = new_save_format;
}

void WorldSave::setSaveDirName(QString dir_name)
{
    QMutexLocker locker(&m_data_lock);

    m_save_dir_name = dir_name;
}

bool WorldSave::valid() const
{
    return m_save_format != WorldSaveFormat::INVALID;
}
