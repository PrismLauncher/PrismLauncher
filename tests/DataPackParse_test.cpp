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

#include <minecraft/mod/DataPack.h>
#include <minecraft/mod/tasks/LocalDataPackParseTask.h>
#include <minecraft/mod/tasks/LocalResourceParse.h>

class DataPackParseTest : public QObject {
    Q_OBJECT

   private slots:
    void test_parseZIP()
    {
        QString source = QFINDTESTDATA("testdata/DataPackParse");

        QString zip_dp = FS::PathCombine(source, "test_data_pack_boogaloo.zip");
        DataPack pack{ QFileInfo(zip_dp) };

        bool valid = DataPackUtils::processZIP(&pack);

        QVERIFY(pack.packFormat() == 4);
        QVERIFY(pack.description() == "Some data pack 2 boobgaloo");
        QVERIFY(valid == true);
    }

    void test_parseFolder()
    {
        QString source = QFINDTESTDATA("testdata/DataPackParse");

        QString folder_dp = FS::PathCombine(source, "test_folder");
        DataPack pack{ QFileInfo(folder_dp) };

        bool valid = DataPackUtils::processFolder(&pack);

        QVERIFY(pack.packFormat() == 10);
        QVERIFY(pack.description() == "Some data pack, maybe");
        QVERIFY(valid == true);
    }

    void test_parseFolder2()
    {
        QString source = QFINDTESTDATA("testdata/DataPackParse");

        QString folder_dp = FS::PathCombine(source, "another_test_folder");
        DataPack pack{ QFileInfo(folder_dp) };

        bool valid = DataPackUtils::process(&pack);

        QVERIFY(pack.packFormat() == 6);
        QVERIFY(pack.description() == "Some data pack three, leaves on the tree");
        QVERIFY(valid == true);
    }

    void test_identifyDataPackZIP()
    {
        QString source = QFINDTESTDATA("testdata/DataPackParse");
        QFileInfo zip_dp(FS::PathCombine(source, "test_data_pack_boogaloo.zip"));
        QVERIFY(DataPackUtils::validate(zip_dp));
        QVERIFY(!DataPackUtils::validateResourcePack(zip_dp));
        auto type = ResourceUtils::identify(zip_dp);
        QCOMPARE(type, ModPlatform::ResourceType::DataPack);
    }

    void test_identifyDataPackFolder()
    {
        QString source = QFINDTESTDATA("testdata/DataPackParse");
        QFileInfo folder_dp(FS::PathCombine(source, "test_folder"));
        QVERIFY(DataPackUtils::validate(folder_dp));
        QVERIFY(!DataPackUtils::validateResourcePack(folder_dp));
        auto type = ResourceUtils::identify(folder_dp);
        QCOMPARE(type, ModPlatform::ResourceType::DataPack);
    }

    void test_identifyDataPackFolder2()
    {
        QString source = QFINDTESTDATA("testdata/DataPackParse");
        QFileInfo folder_dp(FS::PathCombine(source, "another_test_folder"));
        QVERIFY(DataPackUtils::validate(folder_dp));
        QVERIFY(!DataPackUtils::validateResourcePack(folder_dp));
        auto type = ResourceUtils::identify(folder_dp);
        QCOMPARE(type, ModPlatform::ResourceType::DataPack);
    }

    void test_identifyHybridPackZIP()
    {
        QString source = QFINDTESTDATA("testdata/DataPackParse");
        QFileInfo hybrid_zip(FS::PathCombine(source, "test_hybrid_pack.zip"));
        QVERIFY(DataPackUtils::validate(hybrid_zip));
        QVERIFY(!DataPackUtils::validateResourcePack(hybrid_zip));
        auto type = ResourceUtils::identify(hybrid_zip);
        QCOMPARE(type, ModPlatform::ResourceType::DataPack);
    }

    void test_identifyHybridPackFolder()
    {
        QString source = QFINDTESTDATA("testdata/DataPackParse");
        QFileInfo hybrid_folder(FS::PathCombine(source, "hybrid_folder"));
        QVERIFY(DataPackUtils::validate(hybrid_folder));
        QVERIFY(!DataPackUtils::validateResourcePack(hybrid_folder));
        auto type = ResourceUtils::identify(hybrid_folder);
        QCOMPARE(type, ModPlatform::ResourceType::DataPack);
    }

    void test_identifyMcmetaOnlyZIP()
    {
        QString source = QFINDTESTDATA("testdata/DataPackParse");
        QFileInfo mcmeta_only(FS::PathCombine(source, "test_mcmeta_only.zip"));
        QVERIFY(!DataPackUtils::validate(mcmeta_only));
        QVERIFY(DataPackUtils::validateResourcePack(mcmeta_only));
        auto type = ResourceUtils::identify(mcmeta_only);
        QCOMPARE(type, ModPlatform::ResourceType::ResourcePack);
    }

    void test_identifyMcmetaOnlyFolder()
    {
        QString source = QFINDTESTDATA("testdata/DataPackParse");
        QFileInfo mcmeta_only(FS::PathCombine(source, "mcmeta_only_folder"));
        QVERIFY(!DataPackUtils::validate(mcmeta_only));
        QVERIFY(DataPackUtils::validateResourcePack(mcmeta_only));
        auto type = ResourceUtils::identify(mcmeta_only);
        QCOMPARE(type, ModPlatform::ResourceType::ResourcePack);
    }
};

QTEST_GUILESS_MAIN(DataPackParseTest)

#include "DataPackParse_test.moc"
