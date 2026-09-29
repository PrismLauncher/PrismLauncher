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

#include "cli/Parser.h"

#include <algorithm>
#include <cstdlib>

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <variant>

#include <CLI/CLI.hpp>

#include "BuildConfig.h"
#include "cli/Commands.h"
#include "startup/Startup.h"

#include <QUrl>

namespace Cli {
namespace {

struct URLValidator : CLI::Validator {
    URLValidator()
    {
        name_ = "URL";
        func_ = [](const std::string& uri) -> std::string {
            auto uriStr = QString::fromStdString(uri);

            QUrl url = QUrl::fromUserInput(uriStr);

            if (url.isLocalFile()) {
                return { "Is a local file" };
            }

            url.setUrl(uriStr, QUrl::TolerantMode);
            if (!url.isValid()) {
                return url.errorString().toStdString();
            }

            return {};
        };
    }
};

struct LegacyCli {
    CLI::App* launchGroup;
    CLI::Option* launch;
    CLI::Option* server;
    CLI::Option* world;
    CLI::Option* account;
    CLI::Option* offline;
    CLI::Option* showMain;
    CLI::Option* showInstance;
    CLI::Option* alive;
    CLI::Option* import;
};

LegacyCli addLegacyArgs(CLI::App& app, Args& args)
{
    // legacy api
    //
    // { { { "d", "dir" }, "Use a custom path as application root (use '.' for current directory)", "directory" },
    //   { { "l", "launch" }, "Launch the specified instance (by instance ID)", "instance" },
    //   { { "s", "server" }, "Join the specified server on launch (only valid in combination with --launch)", "address" },
    //   { { "w", "world" }, "Join the specified world on launch (only valid in combination with --launch)", "world" },
    //   { { "a", "profile" }, "Use the account specified by its profile name (only valid in combination with --launch)", "profile" },
    //   { { "o", "offline" }, "Launch offline, with given player name (only valid in combination with --launch)", "offline" },
    //   { "alive", "Write a small '" + g_liveCheckFile + "' file after the launcher starts" },
    //   { "show-window", "Show the main launcher window (useful in combination with --launch)" },
    //   { { "I", "import" }, "Import instance or resource from specified local path or URL", "url" },
    //   { "show", "Opens the window for the specified instance (by instance ID)", "show" } });

    LegacyCli l{};

    // mid parse storage
    std::optional<Cmd::Launch> processingLaunch = std::nullopt;
    // legacy launch api
    {
        l.launchGroup = app.add_option_group("launch", "launch an instance")
                            ->preparse_callback([&processingLaunch](std::size_t /*_*/) {
                                // default construct when group encountered
                                processingLaunch.emplace();
                            })
                            ->parse_complete_callback([&args, &processingLaunch]() {
                                if (processingLaunch) {
                                    // push the launch command
                                    args.commands.emplace_back(std::move(processingLaunch.value()));
                                    processingLaunch = std::nullopt;
                                }
                            });
        l.launch = l.launchGroup
                       ->add_option_function<std::string>(
                           "-l,--launch", [&processingLaunch](const std::string& id) { processingLaunch->id = id; },
                           "Launch the specified instance (by instance ID).")
                       ->option_text("id");

        l.server = l.launchGroup
                       ->add_option_function<std::string>(
                           "-s,--server",
                           [&processingLaunch](const std::string& address) {
                               processingLaunch->target = Cmd::Launch::ServerTarget{ .target = address };
                           },
                           "Join the specified server on launch.")
                       ->option_text("address")
                       ->needs(l.launch);

        l.world = l.launchGroup
                      ->add_option_function<std::string>(
                          "-w,--world",
                          [&processingLaunch](const std::string& world) {
                              processingLaunch->target = Cmd::Launch::WorldTarget{ .target = world };
                          },
                          "Join the specified world on launch.")
                      ->option_text("world")
                      ->needs(l.launch)
                      ->excludes(l.server);

        l.server->excludes(l.world);

        l.account = l.launchGroup
                        ->add_option_function<std::string>(
                            "-a,--profile,--account",
                            [&processingLaunch](const std::string& account) {
                                processingLaunch->account = Cmd::Launch::AccountProfile{ .name = account };
                            },
                            "Use the account specified by its profile name.")
                        ->option_text("profile")
                        ->needs(l.launch);

        l.offline =
            l.launchGroup
                ->add_option_function<std::string>(
                    "-o,--offline",
                    [&processingLaunch](const auto& name) { processingLaunch->account = Cmd::Launch::AccountOffline{ .name = name }; },
                    "Launch offline, with given player name.")
                ->option_text("name")
                ->excludes(l.account);

        l.account->excludes(l.offline);
    }

    {
        // legacy show-window
        l.showMain = app.add_flag_function(
            "--show-window",
            [&args](std::int64_t /*count*/) {
                if (std::ranges::none_of(args.commands,
                                         [](const Command& cmd) -> bool { return std::holds_alternative<Cmd::ShowMainWindow>(cmd); })) {
                    args.commands.emplace_back(Cmd::ShowMainWindow{});
                }
            },
            "Show the main launcher window (useful in combination with --launch).");
    }

    {
        // legacy show
        l.showInstance = app.add_option_function<std::string>(
                                "--show",
                                [&args](const std::string& id) {
                                    if (std::ranges::none_of(args.commands, [id](const Command& cmd) -> bool {
                                            return std::holds_alternative<Cmd::ShowInstanceWindow>(cmd) &&
                                                   std::get<Cmd::ShowInstanceWindow>(cmd).id == id;
                                        })) {
                                        args.commands.emplace_back(Cmd::ShowInstanceWindow{ .id = id });
                                    }
                                },
                                "Opens the window for the specified instance (by instance ID).")
                             ->option_text("id");
    }

    {
        // legacy alive
        l.alive = app.add_option_function<std::filesystem::path>(
                         "--alive",
                         [&args](std::filesystem::path path) {
                             if (path.empty()) {
                                 path = std::filesystem::current_path();
                             }
                             path = std::filesystem::absolute(path);
                             if (std::ranges::none_of(args.commands, [&](const Command& cmd) -> bool {
                                     return std::holds_alternative<Cmd::Alive>(cmd) && std::get<Cmd::Alive>(cmd).path == path;
                                 })) {
                                 args.commands.emplace_back(Cmd::Alive{ .path = path });
                             }
                         },
                         std::string{ "Write a small '" } + std::string{ g_liveCheckFile } +
                             "' file after the launcher starts.\n"
                             "The file is written in the directory specified.\n"
                             "(Defaults to the current directory)")
                      ->check(CLI::ExistingDirectory)
                      ->default_str(std::filesystem::current_path().string())
                      ->expected(0, 1);
    }

    {
        // legacy import
        app.add_option_function<std::string>(
               "-I,--import", [&args](const std::string& uri) { args.commands.emplace_back(Cmd::ProcessURI{ .uri = uri }); },
               "Import instance or resource from specified local path or URL")
            ->option_text("url")
            ->check(CLI::ExistingPath | URLValidator{});
    }

    return l;
}

}  // namespace
void parseArgs(int argc, char** argv, Args& args)
{
    CLI::App app{};

    /// ensured that argv is utf8
    /// (only does something on windows, returns argv unchanged everywhere else)
    char** utf8Argv = app.ensure_utf8(argv);

    // set app name and description
    app.name(BuildConfig.LAUNCHER_APP_BINARY_NAME.toStdString());
    app.description(BuildConfig.LAUNCHER_SUMMARY.toStdString());

    app.set_version_flag("--version", BuildConfig.printableVersionString().toStdString());

    std::string dPathSource{};
    switch (args.dataPath.source) {
        case Startup::DataPathSource::SystemEnvironment: {
            dPathSource = QString(" (From `%1`)").arg(QString("%1_DATA_DIR").arg(BuildConfig.LAUNCHER_NAME.toUpper())).toStdString();
            break;
        }
        case Startup::DataPathSource::PortableData:
        case Startup::DataPathSource::PortableUserData: {
            dPathSource = " (portable install)";
            break;
        }
        case Startup::DataPathSource::PersistentDataPath: {
            dPathSource = " (default data dir)";
            break;
        }
        default: {
            break;
        }
    }
    app.add_option_function<std::filesystem::path>(
           "-d,--dir",
           [&args](const std::filesystem::path& dataDir) {
               args.dataPath.dataPath = dataDir;
               args.dataPath.source = Startup::DataPathSource::Commandline;
           },
           "Use a custom path as application root (use '.' for current directory)")
        ->default_str(args.dataPath.dataPath.string() + dPathSource);

    /* auto legacyCli =*/
    addLegacyArgs(app, args);

    // these legacy options can be deprecated once we have a better subcommand based cli in place
    // CLI::deprecate_option(legacyCli.launch, "replacement");
    // CLI::deprecate_option(legacyCli.server, "replacement");
    // CLI::deprecate_option(legacyCli.world, "replacement");
    // CLI::deprecate_option(legacyCli.offline, "replacement");
    // CLI::deprecate_option(legacyCli.account, "replacement");
    // CLI::deprecate_option(legacyCli.import, "replacement");

    app.add_option_function<std::string>(
           "uri", [&args](const std::string& uri) { args.commands.emplace_back(Cmd::ProcessURI{ .uri = uri }); }, "a resource to import")
        ->option_text("uri")
        ->check(CLI::ExistingPath | URLValidator{});

    // try parse or exit
    try {
        app.parse(argc, utf8Argv);
    } catch (const CLI::ParseError& e) {
        // CLI::App::exit prints to stderr with help or parsing errors before
        // returning an exit code
        std::exit(app.exit(e));
    };
}

}  // namespace Cli
