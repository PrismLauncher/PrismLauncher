// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Octol1ttle <l1ttleofficial@outlook.com>
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

#include <QGuiApplication>

#include "OpenGL.h"
#include "Print.h"
#include "Vulkan.h"
#include "lspci.h"

#ifdef Q_OS_WIN32
#include "console/WindowsConsole.h"
#endif

int main(int argc, char* argv[])
{
#ifdef Q_OS_WIN32
    // attach the parent console
    console::WindowsConsoleGuard _consoleGuard;
#endif

    const QGuiApplication app(argc, argv);

    const auto args = std::span{ argv, static_cast<size_t>(argc) };
    for (const std::string_view arg : args.subspan(1)) {
        if (arg == "vulkan") {
            Vulkan::print();
        } else if (arg == "opengl") {
            OpenGL::print();
        } else if (arg == "lspci") {
            lspci::print();
        } else {
            Print::err(QString("Ignoring unknown method: %1").arg(arg));
        }
    }
}
