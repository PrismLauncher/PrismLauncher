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

#include "plugin/PluginContribution.h"

class CatPackContribution : public PluginContribution {
   public:
    CatPackContribution() : PluginContribution(ExtensionPointKind::CAT_PACK) {}

    bool loadConfig(const Plugin& plugin, const QJsonValue& json) override;
    void onPluginEnable() override;
    void onPluginDisable() override;
    bool requiresRestart() override { return false; }

   private:
    QFileInfo m_manifest;
    QString m_id;
};
