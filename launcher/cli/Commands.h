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

#include <filesystem>
#include <string_view>
#include <variant>
#include <vector>

#include "startup/Startup.h"

namespace Cli {

static constexpr std::string_view g_liveCheckFile = "live.check";

namespace Cmd {

struct Launch {
    // targets
    struct NoTarget {};
    struct ServerTarget {
        std::string target;
    };
    struct WorldTarget {
        std::string target;
    };
    using TargetBase = std::variant<NoTarget, ServerTarget, WorldTarget>;
    struct Target : TargetBase {
        using TargetBase::TargetBase;
    };

    // accounts
    struct AccountDefault {};
    struct AccountProfile {
        std::string name;
    };
    struct AccountOffline {
        std::string name;
    };
    using AccountBase = std::variant<AccountDefault, AccountProfile, AccountOffline>;
    struct Account : AccountBase {
        using AccountBase::AccountBase;
    };

    std::string id;
    Target target = NoTarget{};
    Account account = AccountDefault{};
};
struct ProcessURI {
    std::string uri;
};
struct ShowMainWindow {};
struct ShowInstanceWindow {
    std::string id;
};
struct Alive {
    std::filesystem::path path;
};

};  // namespace Cmd

using CommandBase = std::variant<Cmd::Launch, Cmd::ProcessURI, Cmd::ShowMainWindow, Cmd::ShowInstanceWindow, Cmd::Alive>;
struct Command : CommandBase {
    using CommandBase::CommandBase;
};

struct Args {
    Startup::DataPathResult dataPath;
    std::vector<Command> commands;
};

}  // namespace Cli
