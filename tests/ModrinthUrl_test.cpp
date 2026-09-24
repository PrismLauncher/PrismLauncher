#include <QTest>

#include <modplatform/modrinth/ModrinthAPI.h>

class ModrinthUrlTest : public QObject {
    Q_OBJECT

   private slots:
    void parseModpackLink_data()
    {
        QTest::addColumn<QString>("link");
        QTest::addColumn<QString>("slug");

        QTest::newRow("official") << "modrinth://modpack/fabulously-optimized"
                                  << "fabulously-optimized";
        QTest::newRow("trailing slash") << "modrinth://modpack/fabulously-optimized/"
                                        << "fabulously-optimized";
        QTest::newRow("case insensitive host") << "modrinth://MODPACK/fo"
                                               << "fo";
    }

    void parseModpackLink()
    {
        QFETCH(QString, link);
        QFETCH(QString, slug);

        QCOMPARE(ModrinthAPI::getModpackIdFromUrl(QUrl(link)), slug);
    }

    void rejectInvalidLinks_data()
    {
        QTest::addColumn<QString>("link");

        QTest::newRow("wrong scheme") << "https://modrinth.com/modpack/fabulously-optimized";
        QTest::newRow("missing slug") << "modrinth://modpack/";
        QTest::newRow("unsupported resource") << "modrinth://mod/sodium";
        QTest::newRow("extra path") << "modrinth://modpack/fabulously-optimized/version/latest";
    }

    void rejectInvalidLinks()
    {
        QFETCH(QString, link);

        QVERIFY(ModrinthAPI::getModpackIdFromUrl(QUrl(link)).isEmpty());
    }
};

QTEST_GUILESS_MAIN(ModrinthUrlTest)

#include "ModrinthUrl_test.moc"
