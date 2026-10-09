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
#include <ostream>
#include <print>

#include "TestHelpers.h"

int main(int argc, char* argv[])
{
    size_t succeeded = 0;
    size_t testsCount = g_tests.size();

    for (size_t i = 0; i < testsCount; ++i) {
        const auto& [name, func] = g_tests.at(i);
        std::println(std::cout, "Running {} ({}/{})", name, i + 1, testsCount);
        if (int ret = func(argc, argv); ret != 0) {
            std::println(std::cerr, "{} FAILED: application exited with code {}", name, ret);
            continue;
        }

        succeeded++;
    }

    std::println(std::cout, "{}/{} tests have passed", succeeded, testsCount);
    return succeeded == testsCount ? 0 : 1;
}