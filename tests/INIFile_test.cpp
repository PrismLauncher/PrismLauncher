#include <QTest>

#include <QList>
#include <QVariant>
#include <QMap>
#include <QHash>
#include <qtestcase.h>
#include <settings/INIFile.h>

#include <QVariantUtils.h>

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
        QStringList list_strings = {"a", "b", "c", "a,b", "c,d,e", "f\",", "\"g", "h"};

        QList<int> list_numbers = {1, 2, 3, 10};

        QString filename = "test_SaveLoadLists.ini";

        INIFile f;
        f.set("list_strings", list_strings);
        f.set("list_numbers", QVariantUtils::fromList(list_numbers));
        f.saveFile(filename);

        // load
        INIFile f2;
        f2.loadFile(filename);

        QStringList out_list_strings = f2.get("list_strings", QStringList()).toStringList();
        qDebug() << "OutStringList" << out_list_strings;
        
        QList<int> out_list_numbers = QVariantUtils::toList<int>(f2.get("list_numbers", QVariantUtils::fromList(QList<int>())));
        qDebug() << "OutNumbersList" << out_list_numbers;

        QCOMPARE(out_list_strings, list_strings);
        QCOMPARE(out_list_numbers, list_numbers);
    }

    void test_SaveLoadMapHash()
    {
        QMap<QString, QVariant> map;
        map.insert("key_int", 10);
        map.insert("key_str", "This is a String");
        map.insert("key_int_list", QVariantList({1, 2, 3}));
        map.insert("key_string_list", QVariantList({"a", "b", "c"}));

        QHash<QString, QVariant> hash;
        hash.insert("key_int", 20);
        hash.insert("key_str", "This is also String");
        hash.insert("key_int_list", QVariantList({4, 5, 6}));
        hash.insert("key_string_list", QVariantList({"e", "f", "g"}));
        
        QString filename = "test_SaveLoadMapOrHash.ini";
        INIFile f;
        f.set("key_map", map);
        f.set("key_hash", hash);
        f.set("group1/key1", 1);
        f.set("group1/key2", "a");
        f.set("group2/sub1/key1", 2);
        f.set("group2/sub2/key1", "b");
        f.set({"group3", "key1"}, QVariantList({"This is a string", 1000, false}));
        f.saveFile(filename);
        
        // load
        INIFile f2;
        f2.loadFile(filename);

        auto out_map = f2.get("key_map", {}).toMap();
        auto out_hash = f2.get("key_hash", {}).toHash();

        QCOMPARE(out_map, map);
        QCOMPARE(out_hash, hash);
        QCOMPARE(f2.get("group1/key1", 1).toInt(), 1);
        QCOMPARE(f2.get("group1/key2", "a").toString(), "a");
        QCOMPARE(f2.get("group2/sub1/key1", 2).toInt(), 2);
        QCOMPARE(f2.get("group2/sub2/key1", "b").toString(), "b");
        QCOMPARE(f2.get({"group3", "key1"}, QVariantList({})).toList(), QVariantList({"This is a string", 1000, false}));

    }
};

QTEST_GUILESS_MAIN(IniFileTest)

#include "INIFile_test.moc"
