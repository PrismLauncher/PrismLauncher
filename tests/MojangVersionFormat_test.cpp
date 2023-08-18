#include <QDebug>
#include <QTest>

#include <minecraft/MojangVersionFormat.h>

class MojangVersionFormatTest : public QObject {
    Q_OBJECT

    static QJsonDocument readJson(const QString path)
    {
        QFile jsonFile(path);
        jsonFile.open(QIODevice::ReadOnly);
        auto data = jsonFile.readAll();
        jsonFile.close();
        return QJsonDocument::fromJson(data);
    }
    static void writeJson(const char* file, QJsonDocument doc)
    {
        QFile jsonFile(file);
        jsonFile.open(QIODevice::WriteOnly | QIODevice::Text);
        auto data = doc.toJson(QJsonDocument::Indented);
        qDebug() << QString::fromUtf8(data);
        jsonFile.write(data);
        jsonFile.close();
    }

   private slots:
    void test_Through_Simple()
    {
        QJsonDocument doc = readJson(QFINDTESTDATA("testdata/MojangVersionFormat/1.9-simple.json"));
        auto vfile = MojangVersionFormat::versionFileFromJson(doc, "1.9-simple.json");
        auto doc2 = MojangVersionFormat::versionFileToJson(vfile);
        writeJson("1.9-simple-passthorugh.json", doc2);

        QCOMPARE(doc.toJson(), doc2.toJson());
    }

    void test_Through()
    {
        QJsonDocument doc = readJson(QFINDTESTDATA("testdata/MojangVersionFormat/1.9.json"));
        auto vfile = MojangVersionFormat::versionFileFromJson(doc, "1.9.json");
        auto doc2 = MojangVersionFormat::versionFileToJson(vfile);
        writeJson("1.9-passthorugh.json", doc2);
        QCOMPARE(doc.toJson(), doc2.toJson());
    }
};

QTEST_GUILESS_MAIN(MojangVersionFormatTest)

#include "MojangVersionFormat_test.moc"
