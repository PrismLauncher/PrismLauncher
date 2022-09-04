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
#include <QObject>

#include "minecraft/mod/TexturePack.h"

#include "tasks/Task.h"

namespace TexturePackUtils {
bool process(TexturePack& pack);

void processZIP(TexturePack& pack);
void processFolder(TexturePack& pack);

void processPackTXT(TexturePack& pack, QByteArray&& raw_data);
void processPackPNG(TexturePack& pack, QByteArray&& raw_data);
}  // namespace TexturePackUtils

class LocalTexturePackParseTask : public Task {
    Q_OBJECT
   public:
    LocalTexturePackParseTask(int token, TexturePack& rp);

    [[nodiscard]] bool canAbort() const override { return true; }
    bool abort() override;

    void executeTask() override;

    [[nodiscard]] int token() const { return m_token; }

   private:
    int m_token;

    TexturePack& m_resource_pack;

    bool m_aborted = false;
};
