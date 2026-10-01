// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 TheKodeToad <TheKodeToad@proton.me>
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

#include <QString>
#include <cstdint>

#include "Result.h"
#include "config/Config.h"

struct PlayTimeConfig {
    static Result<PlayTimeConfig> load(const QString& path);

    Result<> save(const QString& path) const;

    bool migrated;
    std::int64_t totalPlayTime;

    bool operator==(const PlayTimeConfig&) const = default;
};

class PlayTimeConfigHolder : public ConfigHolder<PlayTimeConfig> {
   public:
    using ConfigHolder::ConfigHolder;
};
