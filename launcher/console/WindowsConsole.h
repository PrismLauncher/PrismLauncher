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
 *
 */

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <system_error>

namespace Console {
void bindCrtHandlesToStdHandles(bool bindStdIn, bool bindStdOut, bool bindStdErr);
bool attachWindowsConsole();
std::error_code enableAnsiSupport();
void freeWindowsConsole();

class WindowsConsoleGuard {
   public:
    WindowsConsoleGuard();
    ~WindowsConsoleGuard();

   private:
    bool m_consoleAttached;
};

}  // namespace console
