// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

bool processZIP(TexturePack& pack);
bool processFolder(TexturePack& pack);

bool processPackTXT(TexturePack& pack, QByteArray&& raw_data);

/// processes ONLY the pack.png (rest of the pack may be invalid)
QPixmap processPackPNG(ResourceType type, const QString& path);

/** Checks whether a file is valid as a texture pack or not. */
bool validate(QFileInfo file);
}  // namespace TexturePackUtils

class LocalTexturePackParseTask : public Task {
    Q_OBJECT
   public:
    LocalTexturePackParseTask(int token, TexturePack& rp);

    bool canAbort() const override { return true; }
    bool abort() override;

    void executeTask() override;

    int token() const { return m_token; }

   private:
    int m_token;

    TexturePack& m_texture_pack;

    bool m_aborted = false;
};
