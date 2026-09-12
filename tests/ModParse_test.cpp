// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2026 Nic <73386479+nicyoong@users.noreply.github.com>
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
#include "FileSystem.h"
#include "minecraft/mod/Mod.h"
#include "minecraft/mod/tasks/LocalModParseTask.h"

// Regression tests for #6047: a malformed mods.toml inside a mod jar used to
// terminate the launcher with an uncaught toml::parse_error. Parsing must
// always come back as "unrecognized mod", never as a crash.
class ModParseTest : public QObject {
    Q_OBJECT

   private slots:
    void validFabricJar()
    {
        QString source = QFINDTESTDATA("testdata/ModParse");
        Mod mod{ FS::PathCombine(source, "valid_fabric.jar") };

        QVERIFY(ModUtils::process(mod, ModUtils::ProcessingLevel::BasicInfoOnly));
        QVERIFY(mod.valid());
        QCOMPARE(mod.name(), "Test Fabric Mod");
        QCOMPARE(mod.version(), "1.2.3");
        QCOMPARE(mod.mod_id(), "test_fabric_mod");
        QCOMPARE(mod.details().authors, QStringList{ "Tester" });
    }

    void validForgeJar()
    {
        QString source = QFINDTESTDATA("testdata/ModParse");
        Mod mod{ FS::PathCombine(source, "valid_forge.jar") };

        QVERIFY(ModUtils::process(mod, ModUtils::ProcessingLevel::BasicInfoOnly));
        QVERIFY(mod.valid());
        QCOMPARE(mod.name(), "Test Forge Mod");
        QCOMPARE(mod.version(), "4.5.6");
        QCOMPARE(mod.mod_id(), "test_forge_mod");
    }

    // A bare key starting with '$' is the exact poison from the issue report.
    void malformedTomlJar()
    {
        QString source = QFINDTESTDATA("testdata/ModParse");
        Mod mod{ FS::PathCombine(source, "malformed_toml.jar") };

        QVERIFY(ModUtils::process(mod, ModUtils::ProcessingLevel::BasicInfoOnly));
        QVERIFY(!mod.valid());
    }

    void truncatedTomlJar()
    {
        QString source = QFINDTESTDATA("testdata/ModParse");
        Mod mod{ FS::PathCombine(source, "truncated_toml.jar") };

        QVERIFY(ModUtils::process(mod, ModUtils::ProcessingLevel::BasicInfoOnly));
        QVERIFY(!mod.valid());
    }

    void jarWithoutMetadata()
    {
        QString source = QFINDTESTDATA("testdata/ModParse");
        Mod mod{ FS::PathCombine(source, "not_a_mod.jar") };

        QVERIFY(!ModUtils::process(mod, ModUtils::ProcessingLevel::BasicInfoOnly));
        QVERIFY(!mod.valid());
    }
};

QTEST_GUILESS_MAIN(ModParseTest)

#include "ModParse_test.moc"
