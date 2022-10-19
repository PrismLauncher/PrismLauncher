// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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

#include <QDebug>
#include "launcherlog.h"
#include <QObject>

#include "minecraft/mod/ResourcePack.h"

#include "tasks/Task.h"

namespace ResourcePackUtils {
bool process(ResourcePack& pack);

void processZIP(ResourcePack& pack);
void processFolder(ResourcePack& pack);

void processMCMeta(ResourcePack& pack, QByteArray&& raw_data);
void processPackPNG(ResourcePack& pack, QByteArray&& raw_data);
}  // namespace ResourcePackUtils

class LocalResourcePackParseTask : public Task {
    Q_OBJECT
   public:
    LocalResourcePackParseTask(int token, ResourcePack& rp);

    [[nodiscard]] bool canAbort() const override { return true; }
    bool abort() override;

    void executeTask() override;

    [[nodiscard]] int token() const { return m_token; }

   private:
    int m_token;

    ResourcePack& m_resource_pack;

    bool m_aborted = false;
};
