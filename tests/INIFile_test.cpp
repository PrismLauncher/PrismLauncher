#include <QTest>

#include <settings/INIFile.h>
#include <QList>
#include <QSettings>
#include <QTemporaryFile>
#include <QVariant>
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
        QString slist_strings = "(\"a\",\"b\",\"c\")";
        QStringList list_strings = { "a", "b", "c" };

        QString slist_numbers = "(1,2,3,10)";
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
