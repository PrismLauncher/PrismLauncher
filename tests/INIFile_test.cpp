#include <QTest>

#include <qtestcase.h>
#include <settings/INIFile.h>
#include <settings/INISettingsObject.h>
#include <QHash>
#include <QList>
#include <QMap>
#include <QSettings>
#include <QTemporaryFile>
#include <QVariant>
#include <memory>
#include "FileSystem.h"

#include <QVariantUtils.h>

class IniFileTest : public QObject {
    Q_OBJECT
   private slots:
    void initTestCase() {}
    void cleanupTestCase() {}

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
        QCOMPARE(f2.get("a", "NOT SET").toString(), a);
        QCOMPARE(f2.get("b", "NOT SET").toString(), b);
    }

    void test_SaveLoadLists()
    {
        QStringList list_strings = { "a", "b", "c", "a,b", "c,d,e", "f\",", "\"g", "h" };

        QList<int> list_numbers = { 1, 2, 3, 10 };

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
        map.insert("key_int_list", QVariantList({ 1, 2, 3 }));
        map.insert("key_string_list", QVariantList({ "a", "b", "c" }));

        QHash<QString, QVariant> hash;
        hash.insert("key_int", 20);
        hash.insert("key_str", "This is also String");
        hash.insert("key_int_list", QVariantList({ 4, 5, 6 }));
        hash.insert("key_string_list", QVariantList({ "e", "f", "g" }));

        QString filename = "test_SaveLoadMapOrHash.ini";
        INIFile f;
        f.set("key_map", map);
        f.set("key_hash", hash);
        f.set("group1/key1", 1);
        f.set("group1/key2", "a");
        f.set("group2/sub1/key1", 2);
        f.set("group2/sub2/key1", "b");
        f.set({ "group3", "key1" }, QVariantList({ "This is a string", 1000, false }));
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
        QCOMPARE(f2.get({ "group3", "key1" }, QVariantList({})).toList(), QVariantList({ "This is a string", 1000, false }));
    }

    void test_SettingsChildGroups()
    {
        QString fileName = "test_settings.cfg";
        {
            auto settings = std::make_shared<INISettingsObject>(fileName);

            settings->setOrRegister("tl_key1", 1);
            settings->setOrRegister("tl_key2", "tld key 2");
            settings->setOrRegister("group1/g1_key1", "g1 key 1");
            settings->setOrRegister({ "group1", "g1_key2" }, "g1 key 2");
            settings->setOrRegister({ "group2", "g2_key1" }, "g2 key 1");
            settings->setOrRegister({ "group2", "g2_key2" }, "g2 key 2");
            settings->setOrRegister("group3/g3_key1", "g3 key 1");
            settings->setOrRegister("group3/g3_key2", "g3 key 2");
            settings->setOrRegister("group3/subgroup1/g3_sg1_key1", 1);
            settings->setOrRegister("group3/subgroup1/g3_sg1_key2", 2);
            settings->setOrRegister("group3/subgroup2/g3_sg2_key1", 3);
            settings->setOrRegister("group3/subgroup2/g3_sg2_key2", 4);
            settings->setOrRegister("group3/subgroup3/g3_sg3_key1", 5);
            settings->setOrRegister("group3/subgroup3/g3_sg3_key2", 6);
            settings->setOrRegister("group4/sub1/sub2/sub3/sub4_key", 100);

            auto tl_keys = settings->childKeys();
            qDebug() << "Top level keys:" << tl_keys;
            QCOMPARE(tl_keys, QStringList({ "tl_key1", "tl_key2" }));
            QCOMPARE(settings->childKeys(""), QStringList({ "tl_key1", "tl_key2" }));
            QCOMPARE(settings->childKeys("group1"), QStringList({ "g1_key1", "g1_key2" }));
            QCOMPARE(settings->childKeys("group2"), QStringList({ "g2_key1", "g2_key2" }));
            QCOMPARE(settings->childKeys("group3"), QStringList({ "g3_key1", "g3_key2" }));
            QCOMPARE(settings->childKeys("group3/subgroup1"), QStringList({ "g3_sg1_key1", "g3_sg1_key2" }));
            QCOMPARE(settings->childKeys("group3/subgroup2"), QStringList({ "g3_sg2_key1", "g3_sg2_key2" }));
            QCOMPARE(settings->childKeys("group3/subgroup3"), QStringList({ "g3_sg3_key1", "g3_sg3_key2" }));
            auto sanity_check = settings->childKeys("group2/subgroup3");
            qDebug() << "Saniity check, should be empty list: group2/subgroup3 ->" << sanity_check;
            QCOMPARE(sanity_check, QStringList({}));
            QCOMPARE(settings->childKeys("group4"), QStringList({}));
            QCOMPARE(settings->childKeys("group4/sub1"), QStringList({}));
            QCOMPARE(settings->childKeys("group4/sub1/sub2"), QStringList({}));
            QCOMPARE(settings->childKeys({ "group4", "sub1", "sub2", "sub3" }), QStringList({ "sub4_key" }));

            auto tl_groups = settings->childGroups();
            qDebug() << "Top level groups:" << tl_groups;
            QCOMPARE(tl_groups, QStringList({ "group1", "group2", "group3", "group4" }));
            QCOMPARE(settings->childGroups(""), QStringList({ "group1", "group2", "group3", "group4" }));
            QCOMPARE(settings->childGroups("group1"), QStringList({}));
            QCOMPARE(settings->childGroups("group2"), QStringList({}));
            QCOMPARE(settings->childGroups("group3"), QStringList({ "subgroup1", "subgroup2", "subgroup3" }));
            QCOMPARE(settings->childGroups({ "group3", "subgroup1" }), QStringList({}));
            QCOMPARE(settings->childGroups({ "group3", "subgroup2" }), QStringList({}));
            QCOMPARE(settings->childGroups("group3/subgroup3"), QStringList({}));
            QCOMPARE(settings->childGroups("group4"), QStringList({ "sub1" }));
            QCOMPARE(settings->childGroups("group4/sub1"), QStringList({ "sub2" }));
            QCOMPARE(settings->childGroups({ "group4", "sub1", "sub2" }), QStringList({ "sub3" }));
            QCOMPARE(settings->childGroups({ "group6", "sub1", "sub2", "sbu3" }), QStringList({}));

            settings->remove("tl_key2");
            QCOMPARE(settings->childKeys(""), QStringList({ "tl_key1" }));

            settings->remove("group1/g1_key2");
            QCOMPARE(settings->childKeys("group1"), QStringList({ "g1_key1" }));

            settings->remove({ "group3", "subgroup2" });
            auto g3cg = settings->childGroups("group3");
            qDebug() << "group3 child groups after subgroup2 removal" << g3cg;
            QCOMPARE(g3cg, QStringList({ "subgroup1", "subgroup3" }));
        }
        qDebug() << "Check the resulting file, ensure correct settings are present";
        INIFile ini_settings;
        ini_settings.loadFile(fileName);

        QCOMPARE(ini_settings.get("tl_key1", 0), QVariant(1));
        QCOMPARE(ini_settings.get("tl_key2", 0), QVariant(0));  // default, so removed
        QCOMPARE(ini_settings.get("group1/g1_key1", 0), QVariant("g1 key 1"));
        QCOMPARE(ini_settings.get({ "group1", "g1_key2" }, 0), QVariant(0));  // default, so removed
        QCOMPARE(ini_settings.get({ "group2", "g2_key1" }, 0), QVariant("g2 key 1"));
        QCOMPARE(ini_settings.get({ "group2", "g2_key2" }, 0), QVariant("g2 key 2"));
        QCOMPARE(ini_settings.get("group3/g3_key1", 0), QVariant("g3 key 1"));
        QCOMPARE(ini_settings.get("group3/g3_key2", 0), QVariant("g3 key 2"));
        QCOMPARE(ini_settings.get("group3/subgroup1/g3_sg1_key1", 0), QVariant(1));
        QCOMPARE(ini_settings.get("group3/subgroup1/g3_sg1_key2", 0), QVariant(2));
        QCOMPARE(ini_settings.get("group3/subgroup2/g3_sg2_key1", 0), QVariant(0));  // default, so removed
        QCOMPARE(ini_settings.get("group3/subgroup2/g3_sg2_key2", 0), QVariant(0));  // default, so removed
        QCOMPARE(ini_settings.get("group3/subgroup3/g3_sg3_key1", 0), QVariant(5));
        QCOMPARE(ini_settings.get("group3/subgroup3/g3_sg3_key2", 0), QVariant(6));
        QCOMPARE(ini_settings.get("group4/sub1/sub2/sub3/sub4_key", 0), QVariant(100));
    }

    void test_SaveAlreadyExistingFile()
    {
        QString fileContent = R"(InstanceType=OneSix
iconKey=vanillia_icon
name=Minecraft Vanillia
OverrideCommands=true
PreLaunchCommand="$INST_JAVA" -jar packwiz-installer-bootstrap.jar link
Wrapperommand=)";
        fileContent += "\"";
        fileContent += +R"(\"$INST_JAVA\" -jar packwiz-installer-bootstrap.jar link =)";
        fileContent += "\"\n";
#if defined(Q_OS_WIN)
        QString fileName = "test_SaveAlreadyExistingFile.ini";
        QFile file(fileName);
        QCOMPARE(file.open(QFile::WriteOnly | QFile::Text), true);
#else
        QTemporaryFile file;
        QCOMPARE(file.open(), true);
        QCOMPARE(file.fileName().isEmpty(), false);
        QString fileName = file.fileName();
#endif
        QTextStream stream(&file);
        stream << fileContent;
        file.close();

        // load
        INIFile f1;
        f1.loadFile(fileName);
        QCOMPARE(f1.get("PreLaunchCommand", "NOT SET").toString(), "\"$INST_JAVA\" -jar packwiz-installer-bootstrap.jar link");
        QCOMPARE(f1.get("Wrapperommand", "NOT SET").toString(), "\"$INST_JAVA\" -jar packwiz-installer-bootstrap.jar link =");
        f1.saveFile(fileName);
        INIFile f2;
        f2.loadFile(fileName);
        QCOMPARE(f2.get("PreLaunchCommand", "NOT SET").toString(), "\"$INST_JAVA\" -jar packwiz-installer-bootstrap.jar link");
        QCOMPARE(f2.get("Wrapperommand", "NOT SET").toString(), "\"$INST_JAVA\" -jar packwiz-installer-bootstrap.jar link =");
        QCOMPARE(f2.get("ConfigVersion", "NOT SET").toString(), "1.2");
#if defined(Q_OS_WIN)
        FS::deletePath(fileName);
#endif
    }

    void test_SaveAlreadyExistingFileWithSpecialChars()
    {
#if defined(Q_OS_WIN)
        QString fileName = "test_SaveAlreadyExistingFileWithSpecialChars.ini";
#else
        QTemporaryFile file;
        QCOMPARE(file.open(), true);
        QCOMPARE(file.fileName().isEmpty(), false);
        QString fileName = file.fileName();
        file.close();
#endif
        QSettings settings{ fileName, QSettings::Format::IniFormat };
        settings.setFallbacksEnabled(false);

        settings.setValue("simple", "value1");
        settings.setValue("withQuotes", R"("value2" with quotes)");
        settings.setValue("withSpecialCharacters", "env mesa=true");
        settings.setValue("withSpecialCharacters2", "1,2,3,4");
        settings.setValue("withSpecialCharacters2", "1;2;3;4");
        settings.setValue("withAll", "val=\"$INST_JAVA\" -jar; ls ");

        settings.sync();

        QCOMPARE(settings.status(), QSettings::Status::NoError);

        // load
        INIFile f1;
        f1.loadFile(fileName);
        for (auto key : settings.allKeys())
            QCOMPARE(f1.get(key, "NOT SET").toString(), settings.value(key).toString());
        f1.saveFile(fileName);
        INIFile f2;
        f2.loadFile(fileName);
        for (auto key : settings.allKeys())
            QCOMPARE(f2.get(key, "NOT SET").toString(), settings.value(key).toString());
        QCOMPARE(f2.get("ConfigVersion", "NOT SET").toString(), "1.2");
#if defined(Q_OS_WIN)
        FS::deletePath(fileName);
#endif
    }

    void test_SaveAlreadyExistingFileWithSpecialCharsV1()
    {
        QString fileContent = R"(InstanceType=OneSix
ConfigVersion=1.1
iconKey=vanillia_icon
name=Minecraft Vanillia
OverrideCommands=true
PreLaunchCommand=)";
        fileContent += "\"\\\"env mesa=true\\\"\"\n";

#if defined(Q_OS_WIN)
        QString fileName = "test_SaveAlreadyExistingFileWithSpecialCharsV1.ini";
        QFile file(fileName);
        QCOMPARE(file.open(QFile::WriteOnly | QFile::Text), true);
#else
        QTemporaryFile file;
        QCOMPARE(file.open(), true);
        QCOMPARE(file.fileName().isEmpty(), false);
        QString fileName = file.fileName();
#endif
        QTextStream stream(&file);
        stream << fileContent;
        file.close();

        // load
        INIFile f1;
        f1.loadFile(fileName);
        QCOMPARE(f1.get("PreLaunchCommand", "NOT SET").toString(), "env mesa=true");
        QCOMPARE(f1.get("ConfigVersion", "NOT SET").toString(), "1.2");
#if defined(Q_OS_WIN)
        FS::deletePath(fileName);
#endif
    }
};

QTEST_GUILESS_MAIN(IniFileTest)

#include "INIFile_test.moc"
