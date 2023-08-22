// SPDX-FileCopyrightText: 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
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

#include "minecraft/mod/WorldSave.h"

#include "tasks/Task.h"

namespace WorldSaveUtils {

enum class ProcessingLevel { Full, BasicInfoOnly };

bool process(WorldSave& save, ProcessingLevel level = ProcessingLevel::Full);

bool processZIP(WorldSave& pack, ProcessingLevel level = ProcessingLevel::Full);
bool processFolder(WorldSave& pack, ProcessingLevel level = ProcessingLevel::Full);

bool validate(QFileInfo file);

}  // namespace WorldSaveUtils

class LocalWorldSaveParseTask : public Task {
    Q_OBJECT
   public:
    LocalWorldSaveParseTask(int token, WorldSave& save);

    [[nodiscard]] bool canAbort() const override { return true; }
    bool abort() override;

    void executeTask() override;

    [[nodiscard]] int token() const { return m_token; }

   private:
    int m_token;

    WorldSave& m_save;

    bool m_aborted = false;
};
