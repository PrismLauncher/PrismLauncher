// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include "FileSystem.h"

#include "minecraft/mod/TexturePack.h"
#include "minecraft/mod/tasks/LocalTexturePackParseTask.h"

class TexturePackParseTest : public QObject {
    Q_OBJECT

    private slots:
    void test_parseZIP()
    {
        QString source = QFINDTESTDATA("testdata/TexturePackParse");

        QString zip_rp = FS::PathCombine(source, "test_texture_pack_idk.zip");
        TexturePack pack { QFileInfo(zip_rp) };

        TexturePackUtils::processZIP(pack);

        QVERIFY(pack.description() == "joe biden, wake up");
    }

    void test_parseFolder()
    {
        QString source = QFINDTESTDATA("testdata/TexturePackParse");

        QString folder_rp = FS::PathCombine(source, "test_texturefolder");
        TexturePack pack { QFileInfo(folder_rp) };

        TexturePackUtils::processFolder(pack);

        QVERIFY(pack.description() == "Some texture pack surely");
    }

    void test_parseFolder2()
    {
        QString source = QFINDTESTDATA("testdata/TexturePackParse");

        QString folder_rp = FS::PathCombine(source, "another_test_texturefolder");
        TexturePack pack { QFileInfo(folder_rp) };

        TexturePackUtils::process(pack);

        QVERIFY(pack.description() == "quieres\nfor real");
    }
};

QTEST_GUILESS_MAIN(TexturePackParseTest)

#include "TexturePackParse_test.moc"
