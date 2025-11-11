// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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

#include <QJsonObject>
#include <QList>
#include <QString>

namespace TechnicSolder {

struct Pack {
    QString recommended;
    QString latest;
    QList<QString> builds;
};

void loadPack(Pack& v, QJsonObject& obj);

struct PackBuildMod {
    QString name;
    QString version;
    QString md5;
    QString url;
};

struct PackBuild {
    QString minecraft;
    QList<PackBuildMod> mods;
};

void loadPackBuild(PackBuild& v, QJsonObject& obj);

}  // namespace TechnicSolder
