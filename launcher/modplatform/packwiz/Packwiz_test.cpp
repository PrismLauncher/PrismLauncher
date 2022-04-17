#include <QTemporaryDir>
#include <QTest>

#include "TestUtil.h"
#include "Packwiz.h"

class PackwizTest : public QObject {
    Q_OBJECT

   private slots:
   // Files taken from https://github.com/packwiz/packwiz-example-pack
   void loadFromFile_Modrinth()
   {
        QString source = QFINDTESTDATA("testdata");

        QDir index_dir(source);
        QString name_mod("borderless-mining.toml");
        QVERIFY(index_dir.entryList().contains(name_mod));

        auto metadata = Packwiz::V1::getIndexForMod(index_dir, name_mod);

        QVERIFY(metadata.isValid());

        QCOMPARE(metadata.name, "Borderless Mining");
        QCOMPARE(metadata.filename, "borderless-mining-1.1.1+1.18.jar");
        QCOMPARE(metadata.side, "client");
        
        QCOMPARE(metadata.url, QUrl("https://cdn.modrinth.com/data/kYq5qkSL/versions/1.1.1+1.18/borderless-mining-1.1.1+1.18.jar"));
        QCOMPARE(metadata.hash_format, "sha512");
        QCOMPARE(metadata.hash, "c8fe6e15ddea32668822dddb26e1851e5f03834be4bcb2eff9c0da7fdc086a9b6cead78e31a44d3bc66335cba11144ee0337c6d5346f1ba63623064499b3188d");

        QCOMPARE(metadata.provider, ModPlatform::Provider::MODRINTH);
        QCOMPARE(metadata.version(), "ug2qKTPR");
        QCOMPARE(metadata.mod_id(), "kYq5qkSL");
   }

    void loadFromFile_Curseforge()
    {
        QString source = QFINDTESTDATA("testdata");

        QDir index_dir(source);
        QString name_mod("screenshot-to-clipboard-fabric.toml");
        QVERIFY(index_dir.entryList().contains(name_mod));

        // Try without the .toml at the end
        name_mod.chop(5);

        auto metadata = Packwiz::V1::getIndexForMod(index_dir, name_mod);

        QVERIFY(metadata.isValid());

        QCOMPARE(metadata.name, "Screenshot to Clipboard (Fabric)");
        QCOMPARE(metadata.filename, "screenshot-to-clipboard-1.0.7-fabric.jar");
        QCOMPARE(metadata.side, "both");
        
        QCOMPARE(metadata.url, QUrl("https://edge.forgecdn.net/files/3509/43/screenshot-to-clipboard-1.0.7-fabric.jar"));
        QCOMPARE(metadata.hash_format, "murmur2");
        QCOMPARE(metadata.hash, "1781245820");

        QCOMPARE(metadata.provider, ModPlatform::Provider::FLAME);
        QCOMPARE(metadata.file_id, 3509043);
        QCOMPARE(metadata.project_id, 327154);
    }
};

QTEST_GUILESS_MAIN(PackwizTest)

#include "Packwiz_test.moc"
