// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

#include "minecraft/mod/ResourcePack.h"

#include "tasks/Task.h"

namespace ResourcePackUtils {

enum class ProcessingLevel { Full, BasicInfoOnly };

bool process(ResourcePack& pack, ProcessingLevel level = ProcessingLevel::Full);

bool processZIP(ResourcePack& pack, ProcessingLevel level = ProcessingLevel::Full);
bool processFolder(ResourcePack& pack, ProcessingLevel level = ProcessingLevel::Full);

QString processComponent(const QJsonValue& value, bool strikethrough = false, bool underline = false);
bool processMCMeta(ResourcePack& pack, QByteArray&& raw_data);
bool processPackPNG(const ResourcePack& pack, QByteArray&& raw_data);

/// processes ONLY the pack.png (rest of the pack may be invalid)
bool processPackPNG(const ResourcePack& pack);

/** Checks whether a file is valid as a resource pack or not. */
bool validate(QFileInfo file);
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
