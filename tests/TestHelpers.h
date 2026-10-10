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

#pragma once

#include <QTest>

// See qtest.h: QTEST_MAIN_WRAPPER where QTEST_BATCH_TESTS is defined
#define REGISTER_TEST(TestObject)                                           \
    namespace {                                                             \
    void register##TestObject()                                             \
    {                                                                       \
        auto runTest = [](int argc, char** argv) -> int {                   \
            const QCoreApplication app(argc, argv);                         \
            TestObject tc;                                                  \
            QTEST_SET_MAIN_SOURCE_PATH                                      \
            return QTest::qExec(&tc, argc, argv);                           \
        };                                                                  \
        g_tests.emplace_back(Test{ .name = #TestObject, .func = runTest }); \
    }                                                                       \
    }                                                                       \
    Q_CONSTRUCTOR_FUNCTION(register##TestObject)  // NOLINT(*-throwing-static-initialization)

struct Test {
    std::string name;
    std::function<int(int, char**)> func;
};

inline std::vector<Test> g_tests{};