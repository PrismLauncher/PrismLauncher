// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 TheKodeToad <TheKodeToad@proton.me>
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

#include "PlayTimeConfig.h"
#include "INIFile.h"

Result<PlayTimeConfig> PlayTimeConfig::load(const QString& path)
{
    INIFile file;
    TRY(file.loadFile(path));

    return PlayTimeConfig{
        .migrated = file.convert("TotalPlayTimeMigrated", false),
        .totalPlayTime = std::max<std::int64_t>(file.convert<qint64>("TotalPlayTime"), 0),
    };
}

Result<> PlayTimeConfig::save(const QString& path) const
{
    qDebug() << u"Saving play time to" << path;

    INIFile file;

    file["TotalPlayTimeMigrated"] = migrated;
    file["TotalPlayTime"] = qint64{ totalPlayTime };

    return file.saveFile(path);
}
