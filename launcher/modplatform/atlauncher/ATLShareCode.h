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
#include <QString>
#include <QVector>

namespace ATLauncher {

struct ShareCodeMod {
    bool selected;
    QString name;
};

struct ShareCode {
    QString pack;
    QString version;
    QVector<ShareCodeMod> mods;
};

struct ShareCodeResponse {
    bool error;
    int code;
    QString message;
    ShareCode data;
};

void loadShareCodeResponse(ShareCodeResponse& r, QJsonObject& obj);

}  // namespace ATLauncher
