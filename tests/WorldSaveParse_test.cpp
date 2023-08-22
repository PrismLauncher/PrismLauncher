
// SPDX-FileCopyrightText: 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
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
 */

#include <QTest>
#include <QTimer>

#include <FileSystem.h>

#include <minecraft/mod/WorldSave.h>
#include <minecraft/mod/tasks/LocalWorldSaveParseTask.h>

class WorldSaveParseTest : public QObject {
    Q_OBJECT

   private slots:
    void test_parseZIP()
    {
        QString source = QFINDTESTDATA("testdata/WorldSaveParse");

        QString zip_ws = FS::PathCombine(source, "minecraft_save_1.zip");
        WorldSave save{ QFileInfo(zip_ws) };

        bool valid = WorldSaveUtils::processZIP(save);

        QVERIFY(save.saveFormat() == WorldSaveFormat::SINGLE);
        QVERIFY(save.saveDirName() == "world_1");
        QVERIFY(valid == true);
    }

    void test_parse_ZIP2()
    {
        QString source = QFINDTESTDATA("testdata/WorldSaveParse");

        QString zip_ws = FS::PathCombine(source, "minecraft_save_2.zip");
        WorldSave save{ QFileInfo(zip_ws) };

        bool valid = WorldSaveUtils::processZIP(save);

        QVERIFY(save.saveFormat() == WorldSaveFormat::MULTI);
        QVERIFY(save.saveDirName() == "world_2");
        QVERIFY(valid == true);
    }

    void test_parseFolder()
    {
        QString source = QFINDTESTDATA("testdata/WorldSaveParse");

        QString folder_ws = FS::PathCombine(source, "minecraft_save_3");
        WorldSave save{ QFileInfo(folder_ws) };

        bool valid = WorldSaveUtils::processFolder(save);

        QVERIFY(save.saveFormat() == WorldSaveFormat::SINGLE);
        QVERIFY(save.saveDirName() == "world_3");
        QVERIFY(valid == true);
    }

    void test_parseFolder2()
    {
        QString source = QFINDTESTDATA("testdata/WorldSaveParse");

        QString folder_ws = FS::PathCombine(source, "minecraft_save_4");
        WorldSave save{ QFileInfo(folder_ws) };

        bool valid = WorldSaveUtils::process(save);

        QVERIFY(save.saveFormat() == WorldSaveFormat::MULTI);
        QVERIFY(save.saveDirName() == "world_4");
        QVERIFY(valid == true);
    }
};

QTEST_GUILESS_MAIN(WorldSaveParseTest)

#include "WorldSaveParse_test.moc"
