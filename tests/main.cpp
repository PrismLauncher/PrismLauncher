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

#include <iostream>

#include "TestHelpers.h"

int main(const int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "No tests specified, nothing to do" << '\n';
        std::cout << "Available tests:" << '\n';
        for (const auto& test : g_tests) {
            std::cout << "  " << test.name << '\n';
        }
        return 1;
    }

    const auto args = std::span{ argv, static_cast<size_t>(argc) };
    bool success = true;

    for (const std::string& arg : args.subspan(1)) {
        if (const auto testIter = std::ranges::find_if(g_tests, [arg](const auto& v) { return v.name == arg; });
            testIter != g_tests.end()) {
            const auto& [name, func] = *testIter;
            std::cout << "Running test " << name << '\n';

            // shorten the args passed to the qt test runner (it tries to parse the test name as test functions list)
            std::string arg0{ argv[0] };
            std::array<char*, 2> qAppArgs = { arg0.data(), nullptr };
            if (const int ret = func(1, qAppArgs.data()); ret != 0) {
                std::cerr << name << " FAILED: application exited with code " << ret << '\n';
                success = false;
            }

            continue;
        }

        std::cerr << "Failed to find test " << arg << '\n';
        return 1;
    }

    return success ? 0 : 1;
}