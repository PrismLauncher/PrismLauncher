// SPDX-FileCopyrightText: 2026 Rachel Powers <508861+Ryex@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Rachel Powers <508861+Ryex@users.noreply.github.com>
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

#include "cli/Commands.h"

#include <CLI/CLI.hpp>
#include <functional>
#include <optional>

namespace Cli {

// struct to hold options and mid parse storage
struct LegacyCli {
    CLI::App* launchGroup{};
    CLI::Option* launch{};
    CLI::Option* server{};
    CLI::Option* world{};
    CLI::Option* account{};
    CLI::Option* offline{};
    CLI::Option* showMain{};
    CLI::Option* showInstance{};
    CLI::Option* alive{};
    CLI::Option* import{};
    std::optional<Cmd::Launch> processingLaunch;

    void addLegacyArgs(CLI::App& app, Args& args);
};

struct Cli {
    LegacyCli legacyCli{};


    void attach(CLI::App& app, Args& args);
};

void parseArgs(int argc, char** argv, Args& args, std::optional<std::function<void(CLI::App&)>> beforeParse = std::nullopt);
}  // namespace Cli
