// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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

#include <QTest>
#include "MMCTime.h"

class TimeTest : public QObject {
    Q_OBJECT

   private slots:
    void test_humanReadableDuration()
    {
        struct testCase {
            const int64_t duration;
            const QString fmt;
            const QString expected;
        };
        for (auto t : {
                 testCase{ 1, "%ssec", "1sec" },
                 { 90, "%M.1min", "1.5min" },
                 { 59, "%s.1sec", "59.0sec" },
                 { 90, "%ssec", "30sec" },
                 { 60 * 60 * 30, "%hh", "6h" },
                 { 60 * 60 * 30, "%Hh", "30h" },
                 { 60 * 60 * 30, "%ddays", "1days" },
                 { 60 * 60 * 30, "%d.1days", "1.3days" },
                 { 60 * 60 * 30, "%d.2days", "1.25days" },

                 { 90, "%dd %hh %mmin %ssec", "1min 30sec" },
                 { 30, "%dd %hh %mmin %ssec", "30sec" },
                 { 30, "%dd %hh %mmin", "0d 0h 0min" },
                 { 90 * 60, "%dd %hh %mmin %ssec", "1h 30min" },
                 { 90 * 60 + 20, "%dd %hh %mmin %ssec", "1h 30min 20sec" },
                 { 60 * 60 * 24 * 2 + 90 * 60 + 20, "%dd %hh %mmin %ssec", "2d 1h 30min 20sec" },

                 { 60 * 60 * 60, "%d.2d", "2.50d" },
                 { 60 * 60 * 60 + 30 * 60, "%H.2h", "60.50h" },
                 { 60 * 61 + 15, "%M.2min", "61.25min" },
                 { 90, "%Ssec", "90sec" },
                 { 90, "", "1min 30s" },
                 { 90, "%%", "%" },
                 { 90, "custom text", "custom text" },
                 { 90, "custom text =%p=", "custom text ==" },
                 { 90, "custom text =%p.12=", "custom text ==" },
                 { 90, "custom text =%p.=", "custom text =.=" },
             }) {
            auto formated = Time::humanReadableDuration(t.duration, t.fmt);
            QCOMPARE(formated, t.expected);
        }
    }
};

QTEST_GUILESS_MAIN(TimeTest)

#include "Time_test.moc"
