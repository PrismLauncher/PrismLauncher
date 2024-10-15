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

#include <cpptrace/utils.hpp>
#include <csignal>
#include <fstream>
#include "Application.h"
#include "FileSystem.h"

// #define BREAK_INFINITE_LOOP
// #define BREAK_EXCEPTION
// #define BREAK_RETURN

#ifdef BREAK_INFINITE_LOOP
#include <chrono>
#include <thread>
#endif

void signal_handler(int)
{
    auto trace = cpptrace::generate_trace();
    auto data = QString::fromStdString(trace.to_string());
    qCritical() << "==========================";
    qCritical() << data;
    FS::write("crash.report", data.toUtf8());
    QFile file("crash2.report");
    if (file.open(QFile::WriteOnly)) {
        file.write(data.toUtf8());
        file.flush();
        file.close();
    }
    std::ofstream MyFile("crash3.report");
    // Write to the file
    trace.print(MyFile);

    // Close the file
    MyFile.close();
    QApplication::exit(1);
}
#if defined Q_OS_WIN32
#include <windows.h>
void HandleException(DWORD exceptionCode)
{
    signal_handler(int);
}
#endif

void setup_crash_handler()
{
    // Setup signal handler for common crash signals
    std::signal(SIGSEGV, signal_handler);  // Segmentation fault
    std::signal(SIGABRT, signal_handler);  // Abort signal
}

void warmup_cpptrace()
{
    cpptrace::frame_ptr buffer[10];
    std::size_t count = cpptrace::safe_generate_raw_trace(buffer, 10);
    cpptrace::safe_object_frame frame;
    cpptrace::get_safe_object_frame(buffer[0], &frame);
}

int main(int argc, char* argv[])
{
#ifdef BREAK_INFINITE_LOOP
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
#endif
#ifdef BREAK_EXCEPTION
    throw 42;
#endif
#ifdef BREAK_RETURN
    return 42;
#endif

    // cpptrace::absorb_trace_exceptions(false);
    cpptrace::register_terminate_handler();
    warmup_cpptrace();
    setup_crash_handler();

#if defined Q_OS_WIN32
    __try {
#endif

#if QT_VERSION <= QT_VERSION_CHECK(6, 0, 0)
        QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

        // initialize Qt
        Application app(argc, argv);

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
                return app.exec();
            }
            case Application::Failed:
                return 1;
            case Application::Succeeded:
                return 0;
            default:
                return -1;
        }
#if defined Q_OS_WIN32
    } __except (HandleException(GetExceptionCode()), EXCEPTION_EXECUTE_HANDLER) {
        std::cout << "Exception handled!" << std::endl;
    }
#endif
}
