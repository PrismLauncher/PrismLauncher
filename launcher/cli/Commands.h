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
    struct NoTarget {
        auto operator<=>(const NoTarget&) const = default;
    };
    struct ServerTarget {
        std::string target;
        auto operator<=>(const ServerTarget&) const = default;
    };
    struct WorldTarget {
        std::string target;
        auto operator<=>(const WorldTarget&) const = default;
    };
    using TargetBase = std::variant<NoTarget, ServerTarget, WorldTarget>;
    struct Target : TargetBase {
        using TargetBase::TargetBase;
    };

    // accounts
    struct AccountDefault {
        auto operator<=>(const AccountDefault&) const = default;
    };
    struct AccountProfile {
        std::string name;
        auto operator<=>(const AccountProfile&) const = default;
    };
    struct AccountOffline {
        std::string name;
        auto operator<=>(const AccountOffline&) const = default;
    };
    using AccountBase = std::variant<AccountDefault, AccountProfile, AccountOffline>;
    struct Account : AccountBase {
        using AccountBase::AccountBase;
    };

    std::string id;
    Target target = NoTarget{};
    Account account = AccountDefault{};
    auto operator<=>(const Launch&) const = default;
};
struct ProcessURI {
    std::string uri;
    auto operator<=>(const ProcessURI&) const = default;
};
struct ShowMainWindow {
    auto operator<=>(const ShowMainWindow&) const = default;
};
struct ShowInstanceWindow {
    std::string id;
    auto operator<=>(const ShowInstanceWindow&) const = default;
};
struct Alive {
    std::filesystem::path path;
    auto operator<=>(const Alive&) const = default;
};

};  // namespace Cmd

using CommandBase = std::variant<Cmd::Launch, Cmd::ProcessURI, Cmd::ShowMainWindow, Cmd::ShowInstanceWindow, Cmd::Alive>;
struct Command : CommandBase {
    using CommandBase::CommandBase;
};

struct Args {
    Startup::DataPathResult dataPath;
    std::vector<Command> commands;
    auto operator<=>(const Args&) const = default;
};

}  // namespace Cli
