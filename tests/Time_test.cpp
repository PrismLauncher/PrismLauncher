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
        QCOMPARE(Time::humanReadableDuration(1, "%s%sec"), "1sec");
        QCOMPARE(Time::humanReadableDuration(90, "%M.1%min"), "1.5min");
        QCOMPARE(Time::humanReadableDuration(59, "%s.1%sec"), "59.0sec");
        QCOMPARE(Time::humanReadableDuration(90, "%s%sec"), "30sec");
        QCOMPARE(Time::humanReadableDuration(60 * 60 * 30, "%h%h"), "6h");
        QCOMPARE(Time::humanReadableDuration(60 * 60 * 30, "%H%h"), "30h");
        QCOMPARE(Time::humanReadableDuration(60 * 60 * 30, "%d%days"), "1days");
        QCOMPARE(Time::humanReadableDuration(60 * 60 * 30, "%d.1%days"), "1.3days");
        QCOMPARE(Time::humanReadableDuration(60 * 60 * 30, "%d.2%days"), "1.25days");

        QCOMPARE(Time::humanReadableDuration(90, "%d%d %h%h %m%min %s%sec"), "1min 30sec");
        QCOMPARE(Time::humanReadableDuration(30, "%d%d %h%h %m%min %s%sec"), "30sec");
        QCOMPARE(Time::humanReadableDuration(30, "%d%d %h%h %m%min"), "");
        QCOMPARE(Time::humanReadableDuration(90 * 60, "%d%d %h%h %m%min %s%sec"), "1h 30min");
        QCOMPARE(Time::humanReadableDuration(90 * 60 + 20, "%d%d %h%h %m%min %s%sec"), "1h 30min 20sec");
        QCOMPARE(Time::humanReadableDuration(60 * 60 * 24 * 2 + 90 * 60 + 20, "%d%d %h%h %m%min %s%sec"), "2d 1h 30min 20sec");

        QCOMPARE(Time::humanReadableDuration(60 * 60 * 24 * 2.5, "%d.2%d"), "2.50d");
        QCOMPARE(Time::humanReadableDuration(60 * 60 * 24 * 2.5 + 30 * 60, "%H.2%h"), "60.50h");
        QCOMPARE(Time::humanReadableDuration(60 * 61 + 15, "%M.2%min"), "61.25min");
        QCOMPARE(Time::humanReadableDuration(90, "%S%sec"), "90sec");
    }
};

QTEST_GUILESS_MAIN(TimeTest)

#include "Time_test.moc"
