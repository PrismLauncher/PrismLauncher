// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include <exception>

#include "crash_handler/CrashHandler.h"
#include "crash_handler/CrashHandlerDialog.h"

#include <BuildConfig.h>

#include <QObject>
#include <QString>

#include "Application.h"
#include "CLI/CLI.hpp"
#include "cli/Commands.h"

#if defined Q_OS_WIN32
#include "console/WindowsConsole.h"
#endif

#include "cli/Commands.h"
#include "cli/Parser.h"

#include "startup/Startup.h"

int main(int argc, char* argv[])
{
#ifdef Q_OS_WIN32
    // used on Windows to attach the standard IO streams
    const Console::WindowsConsoleGuard consoleGuard;
#endif

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const auto exePath = Startup::resolveApplicationFilePath(argv[0]);
    // get default dataPath before cli alterations
    const auto dataPathResult = Startup::resolveDataPath(exePath.parent_path());

    const std::string crashHandlerFlag = "--crash-handler";

    CrashHandler::attach(CrashHandler::CrashConfig{
        .crashHandlerFlag = crashHandlerFlag,
        .processExePath = { exePath.string() },
        .dataPath = { dataPathResult.dataPath.string() },
    });

    Cli::Args args{
        .dataPath = dataPathResult,
        .commands = {},
    };

    bool handleCrash{ false };
    try {
        Cli::parseArgs(argc, argv, args, { [&crashHandlerFlag, &handleCrash](CLI::App& parser) {
                           // add flag and hide it by giving it an empty group
                           parser.add_flag(crashHandlerFlag, handleCrash, "(for internal use) trace a crash trace from stdin")->group("");
                       } });
    } catch (const std::exception& err) {
        /// we did a bad job setting up the cli parser
        std::cerr << "BUG! Failed to parse cli args: " << err.what();
        return 1;
    }

    if (handleCrash) {
        std::cerr << "HANDLING CRASH!\n";

        auto trace = CrashHandler::readTraceFromStdin();

        // std::cerr << "stacktrace:\n\n" << trace << "\n";

        QApplication crashApp(argc, argv);
        QString msg = QObject::tr("%1 has Crashed").arg(BuildConfig.LAUNCHER_DISPLAYNAME);
        CrashHandler::CrashHandlerDialog crashDialog(nullptr, msg, msg, std::move(trace));

        crashDialog.show();

        QApplication::exec();

        std::exit(0);
    }

    // initialize Qt
    Application app(argc, argv, args);
    switch (app.status()) {
        case Application::StartingUp:
        case Application::Initialized: {
            Q_INIT_RESOURCE(multimc);
            Q_INIT_RESOURCE(backgrounds);
            Q_INIT_RESOURCE(documents);
            Q_INIT_RESOURCE(prismlauncher);

            Q_INIT_RESOURCE(pe_dark);
            Q_INIT_RESOURCE(pe_light);
            Q_INIT_RESOURCE(pe_blue);
            Q_INIT_RESOURCE(pe_colored);
            Q_INIT_RESOURCE(breeze_dark);
            Q_INIT_RESOURCE(breeze_light);
            Q_INIT_RESOURCE(OSX);
            Q_INIT_RESOURCE(iOS);
            Q_INIT_RESOURCE(flat);
            Q_INIT_RESOURCE(flat_white);

            Q_INIT_RESOURCE(shaders);
            return Application::exec();
        }
        case Application::Failed:
            return 1;
        case Application::Succeeded:
            return 0;
        default:
            return -1;
    }
}
