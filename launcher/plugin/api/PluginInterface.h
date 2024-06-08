// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Mai Lapyst <67418776+Mai-Lapyst@users.noreply.github.com>
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
 */
#pragma once

#include <QObject>

#include "PluginInstance.h"

class Plugin;

class PluginInterface {
   public:
    virtual ~PluginInterface() = default;

    virtual void onEnable(const PluginInstance& plugin) = 0;
    virtual void onDisable(const PluginInstance& plugin) = 0;

    virtual bool requiresRestart() { return true; }
};

QT_BEGIN_NAMESPACE

#define PluginInterface_iid "org.prismlauncher.PrismLauncher.PluginInterface"

Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)

QT_END_NAMESPACE
