
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

#include <minecraft/mod/ShaderPack.h>
#include <minecraft/mod/tasks/LocalShaderPackParseTask.h>

class ShaderPackParseTest : public QObject {
    Q_OBJECT

   private slots:
    void test_parseZIP()
    {
        QString source = QFINDTESTDATA("testdata/ShaderPackParse");

        QString zip_sp = FS::PathCombine(source, "shaderpack1.zip");
        ShaderPack pack{ QFileInfo(zip_sp) };

        bool valid = ShaderPackUtils::processZIP(pack);

        QVERIFY(pack.packFormat() == ShaderPackFormat::VALID);
        QVERIFY(valid == true);
    }

    void test_parseFolder()
    {
        QString source = QFINDTESTDATA("testdata/ShaderPackParse");

        QString folder_sp = FS::PathCombine(source, "shaderpack2");
        ShaderPack pack{ QFileInfo(folder_sp) };

        bool valid = ShaderPackUtils::processFolder(pack);

        QVERIFY(pack.packFormat() == ShaderPackFormat::VALID);
        QVERIFY(valid == true);
    }

    void test_parseZIP2()
    {
        QString source = QFINDTESTDATA("testdata/ShaderPackParse");

        QString folder_sp = FS::PathCombine(source, "shaderpack3.zip");
        ShaderPack pack{ QFileInfo(folder_sp) };

        bool valid = ShaderPackUtils::process(pack);

        QVERIFY(pack.packFormat() == ShaderPackFormat::INVALID);
        QVERIFY(valid == false);
    }
};

QTEST_GUILESS_MAIN(ShaderPackParseTest)

#include "ShaderPackParse_test.moc"
