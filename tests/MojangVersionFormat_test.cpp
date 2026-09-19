#include <QDebug>
#include <QTest>
#include "Json.h"

#include <minecraft/MojangVersionFormat.h>

class MojangVersionFormatTest : public QObject {
    Q_OBJECT

   private slots:
    void test_Through_Simple()
    {
        auto doc = Json::requireDocument(QFINDTESTDATA("testdata/Libraries/1.9-simple.json"));
        QVERIFY2(doc, doc.has_value() ? "" : qPrintable(doc.error()));

        auto vfile = MojangVersionFormat::versionFileFromJson(doc.value(), "1.9-simple.json");
        QVERIFY2(vfile, vfile.has_value() ? "" : qPrintable(vfile.error()));

        auto doc2 = MojangVersionFormat::versionFileToJson(vfile.value());
        auto wr = Json::write(doc2, "1.9-simple-passthorugh.json");
        QVERIFY2(wr, wr.has_value() ? "" : qPrintable(wr.error()));

        QCOMPARE(doc->toJson(), doc2.toJson());
    }

    void test_Through()
    {
        auto doc = Json::requireDocument(QFINDTESTDATA("testdata/Libraries/1.9.json"));
        QVERIFY2(doc, doc.has_value() ? "" : qPrintable(doc.error()));

        auto vfile = MojangVersionFormat::versionFileFromJson(doc.value(), "1.9.json");
        QVERIFY2(vfile, vfile.has_value() ? "" : qPrintable(vfile.error()));

        auto doc2 = MojangVersionFormat::versionFileToJson(vfile.value());
        auto wr = Json::write(doc2, "1.9-passthorugh.json");
        QVERIFY2(wr, wr.has_value() ? "" : qPrintable(wr.error()));

        QCOMPARE(doc->toJson(), doc2.toJson());
    }
};

QTEST_GUILESS_MAIN(MojangVersionFormatTest)

#include "MojangVersionFormat_test.moc"
