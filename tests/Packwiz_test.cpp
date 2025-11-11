// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

#include <QTemporaryDir>
#include <QTest>
#include "modplatform/ModIndex.h"

#include <modplatform/packwiz/Packwiz.h>

class PackwizTest : public QObject {
    Q_OBJECT

   private slots:
    // Files taken from https://github.com/packwiz/packwiz-example-pack
    void loadFromFile_Modrinth()
    {
        QString source = QFINDTESTDATA("testdata/Packwiz");

        QDir index_dir(source);
        QString slug_mod("borderless-mining");
        QString file_name = slug_mod + ".pw.toml";
        QVERIFY(index_dir.entryList().contains(file_name));

        auto metadata = Packwiz::V1::getIndexForMod(index_dir, slug_mod);

        QVERIFY(metadata.isValid());

        QCOMPARE(metadata.name, "Borderless Mining");
        QCOMPARE(metadata.filename, "borderless-mining-1.1.1+1.18.jar");
        QCOMPARE(metadata.side, ModPlatform::Side::ClientSide);

        QCOMPARE(metadata.url, QUrl("https://cdn.modrinth.com/data/kYq5qkSL/versions/1.1.1+1.18/borderless-mining-1.1.1+1.18.jar"));
        QCOMPARE(metadata.hash_format, "sha512");
        QCOMPARE(metadata.hash,
                 "c8fe6e15ddea32668822dddb26e1851e5f03834be4bcb2eff9c0da7fdc086a9b6cead78e31a44d3bc66335cba11144ee0337c6d5346f1ba6362306449"
                 "9b3188d");

        QCOMPARE(metadata.provider, ModPlatform::ResourceProvider::MODRINTH);
        QCOMPARE(metadata.version(), "ug2qKTPR");
        QCOMPARE(metadata.mod_id(), "kYq5qkSL");
    }

    void loadFromFile_Curseforge()
    {
        QString source = QFINDTESTDATA("testdata/Packwiz");

        QDir index_dir(source);
        QString name_mod("screenshot-to-clipboard-fabric.pw.toml");
        QVERIFY(index_dir.entryList().contains(name_mod));

        // Try without the .pw.toml at the end
        name_mod.chop(8);

        auto metadata = Packwiz::V1::getIndexForMod(index_dir, name_mod);

        QVERIFY(metadata.isValid());

        QCOMPARE(metadata.name, "Screenshot to Clipboard (Fabric)");
        QCOMPARE(metadata.filename, "screenshot-to-clipboard-1.0.7-fabric.jar");
        QCOMPARE(metadata.side, ModPlatform::Side::UniversalSide);

        QCOMPARE(metadata.url, QUrl("https://edge.forgecdn.net/files/3509/43/screenshot-to-clipboard-1.0.7-fabric.jar"));
        QCOMPARE(metadata.hash_format, "murmur2");
        QCOMPARE(metadata.hash, "1781245820");

        QCOMPARE(metadata.provider, ModPlatform::ResourceProvider::FLAME);
        QCOMPARE(metadata.file_id, 3509043);
        QCOMPARE(metadata.project_id, 327154);
    }
};

QTEST_GUILESS_MAIN(PackwizTest)

#include "Packwiz_test.moc"
