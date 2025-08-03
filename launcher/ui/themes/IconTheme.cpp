// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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

#include "IconTheme.h"

#include <QFile>
#include <QSettings>

bool IconTheme::load()
{
    const QString path = m_path + "/index.theme";

    if (!QFile::exists(path))
        return false;

    QSettings settings(path, QSettings::IniFormat);
    settings.beginGroup("Icon Theme");
    m_name = settings.value("Name").toString();
    settings.endGroup();
    return !m_name.isNull();
}
