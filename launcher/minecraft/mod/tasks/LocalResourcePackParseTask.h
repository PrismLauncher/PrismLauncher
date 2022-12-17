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

#include "minecraft/mod/ResourcePack.h"

#include "tasks/Task.h"

namespace ResourcePackUtils {

enum class ProcessingLevel { Full, BasicInfoOnly };

bool process(ResourcePack& pack, ProcessingLevel level = ProcessingLevel::Full);

void processZIP(ResourcePack& pack, ProcessingLevel level = ProcessingLevel::Full);
void processFolder(ResourcePack& pack, ProcessingLevel level = ProcessingLevel::Full);

void processMCMeta(ResourcePack& pack, QByteArray&& raw_data);
void processPackPNG(ResourcePack& pack, QByteArray&& raw_data);

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

    [[nodiscard]] int token() const { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_token; }

   private:
    int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_token;

    ResourcePack& hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_resource_pack;

    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted = false;
};
