// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Vishrut Sachan <vishrutsachan2004@gmail.com>
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

#include <QList>
#include <QString>

#include <functional>

#include "net/NetJob.h"

class MinecraftInstance;

namespace Realms {

struct Realm {
    QString id;
    QString name;
    QString owner;
    QString motd;
    bool open = false;
    bool expired = false;
};

NetJob::Ptr fetch(MinecraftInstance* instance, QObject* context, std::function<void(const QList<Realm>&)> onSucceeded);

}  // namespace Realms
