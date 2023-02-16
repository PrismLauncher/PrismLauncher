#include <QTest>

#include <QList>
#include <QVariant>
#include <settings/INIFile.h>


class IniFileTest : public QObject
{
    Q_OBJECT
private
slots:
    void initTestCase()
    {

    }
    void cleanupTestCase()
    {

    }

    void test_Escape_data()
    {
        QTest::addColumn<QString>("through");

        QTest::newRow("unix path") << "/abc/def/ghi/jkl";
        QTest::newRow("windows path") << "C:\\Program files\\terrible\\name\\of something\\";
        QTest::newRow("Plain text") << "Lorem ipsum dolor sit amet.";
        QTest::newRow("Escape sequences") << "Lorem\n\t\n\\n\\tAAZ\nipsum dolor\n\nsit amet.";
        QTest::newRow("Escape sequences 2") << "\"\n\n\"";
        QTest::newRow("Hashtags") << "some data#something";
    }
    void test_Escape()
    {
        QFETCH(QString, through);

        QString there = INIFile::escape(through);
        QString back = INIFile::unescape(there);

        QCOMPARE(back, through);
    }

    void test_SaveLoad()
    {
        QString a = "a";
        QString b = "a\nb\t\n\\\\\\C:\\Program files\\terrible\\name\\of something\\#thisIsNotAComment";
        QString filename = "test_SaveLoad.ini";

        // save
        INIFile f;
        f.set("a", a);
        f.set("b", b);
        f.saveFile(filename);

        // load
        INIFile f2;
        f2.loadFile(filename);
        QCOMPARE(f2.get("a","NOT SET").toString(), a);
        QCOMPARE(f2.get("b","NOT SET").toString(), b);
    }

    void test_SaveLoadLists()
    {
        QString slist_strings = "[\"a\",\"b\",\"c\"]";
        QStringList list_strings = {"a", "b", "c"};

        QString slist_numbers = "[1,2,3,10]";
        QList<int> list_numbers = {1, 2, 3, 10};

        QString filename = "test_SaveLoadLists.ini";

        INIFile f;
        f.setList("list_strings", list_strings);
        f.setList("list_numbers", list_numbers);
        f.saveFile(filename);

        // load
        INIFile f2;
        f2.loadFile(filename);

        QStringList out_list_strings = f2.getList<QString>("list_strings", QStringList());
        qDebug() << "OutStringList" << out_list_strings;
        
        QList<int> out_list_numbers = f2.getList<int>("list_numbers", QList<int>());
        qDebug() << "OutNumbersList" << out_list_numbers;

        QCOMPARE(f2.get("list_strings","NOT SET").toString(), slist_strings);
        QCOMPARE(out_list_strings, list_strings);
        QCOMPARE(f2.get("list_numbers","NOT SET").toString(), slist_numbers);
        QCOMPARE(out_list_numbers, list_numbers);
    }
};

QTEST_GUILESS_MAIN(IniFileTest)

#include "INIFile_test.moc"
