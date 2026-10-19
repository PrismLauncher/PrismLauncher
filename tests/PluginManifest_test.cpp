#include <QTemporaryDir>
#include <QTest>

#include <plugins/PluginManifest.h>

class PluginManifestTest : public QObject {
    Q_OBJECT

    static QByteArray manifest(const QByteArray& extra = {}, const QByteArray& api = "api = 1")
    {
        return "id = \"com.example.autobackup\"\n"
               "name = \"Auto Backup\"\n"
               "version = \"1.0.0\"\n" +
               api + "\n" + extra;
    }

    static void expectError(const QByteArray& contents, const QString& expected)
    {
        auto result = PluginManifest::parse(contents);
        QVERIFY2(!result, "parsing should have failed");
        QVERIFY2(result.error().contains(expected), qPrintable(result.error()));
    }

   private slots:
    void test_fullManifest()
    {
        auto result = PluginManifest::parse(R"(
            id = "com.example.autobackup"
            name = "Auto Backup"
            version = "1.0.0"
            api = 1
            entry = "src/main.lua"
            permissions = ["launch.hooks", "instances.read", "fs.instance", "instances.read"]
        )");
        QVERIFY2(result, qPrintable(result.error_or("")));
        QCOMPARE(result->id, "com.example.autobackup");
        QCOMPARE(result->name, "Auto Backup");
        QCOMPARE(result->version, "1.0.0");
        QCOMPARE(result->api, 1);
        QCOMPARE(result->entry, "src/main.lua");
        QCOMPARE(result->permissions, QStringList({ "fs.instance", "instances.read", "launch.hooks" }));
        QVERIFY(result->hasPermission("fs.instance"));
        QVERIFY(!result->hasPermission("net"));
    }

    void test_defaults()
    {
        auto result = PluginManifest::parse(manifest());
        QVERIFY2(result, qPrintable(result.error_or("")));
        QCOMPARE(result->entry, "main.lua");
        QVERIFY(result->permissions.isEmpty());
    }

    void test_invalidToml()
    {
        auto result = PluginManifest::parse("id = \"x\"\nversion = 1.0.0\n");
        QVERIFY(!result);
        QVERIFY2(result.error().startsWith("invalid TOML at line 2:"), qPrintable(result.error()));
    }

    void test_missingFields()
    {
        expectError("name = \"x\"\nversion = \"1\"\napi = 1", "missing required field 'id'");
        expectError("id = \"x\"\nversion = \"1\"\napi = 1", "missing required field 'name'");
        expectError("id = \"x\"\nname = \"x\"\napi = 1", "missing required field 'version'");
        expectError(manifest({}, {}), "missing required field 'api'");
    }

    void test_wrongTypes()
    {
        expectError("id = 1\nname = \"x\"\nversion = \"1\"\napi = 1", "field 'id' must be a string");
        expectError(manifest({}, "api = \"1\""), "field 'api' must be an integer");
        expectError(manifest({}, "api = 1.0"), "field 'api' must be an integer");
        expectError(manifest("entry = 3"), "field 'entry' must be a string");
        expectError(manifest("permissions = \"net\""), "field 'permissions' must be an array of strings");
        expectError(manifest("permissions = [1]"), "field 'permissions' must be an array of strings");
    }

    void test_unsupportedApi()
    {
        expectError(manifest({}, "api = 2"), "unsupported plugin API 2");
        expectError(manifest({}, "api = 0"), "unsupported plugin API 0");
    }

    void test_unknownFieldsAndPermissions()
    {
        expectError(manifest("permision = [\"net\"]"), "unknown field 'permision'");
        expectError(manifest("permissions = [\"accounts\"]"), "unknown permission 'accounts'");
    }

    void test_emptyStrings()
    {
        expectError("id = \"x\"\nname = \"  \"\nversion = \"1\"\napi = 1", "field 'name' must not be empty");
        expectError("id = \"x\"\nname = \"x\"\nversion = \"\"\napi = 1", "field 'version' must not be empty");
        expectError(manifest("entry = \"\""), "invalid entry ''");
    }

    void test_id_data()
    {
        QTest::addColumn<QString>("id");
        QTest::addColumn<bool>("valid");

        QTest::newRow("reverse dns") << "com.example.auto-backup_2" << true;
        QTest::newRow("single segment") << "backup" << true;
        QTest::newRow("max length") << QString(64, 'a') << true;
        QTest::newRow("too long") << QString(65, 'a') << false;
        QTest::newRow("empty") << "" << false;
        QTest::newRow("uppercase") << "Com.Example" << false;
        QTest::newRow("empty segment") << "com..example" << false;
        QTest::newRow("leading dot") << ".hidden" << false;
        QTest::newRow("trailing dot") << "com.example." << false;
        QTest::newRow("dot dot") << ".." << false;
        QTest::newRow("slash") << "com/example" << false;
        QTest::newRow("backslash") << "com\\\\example" << false;
        QTest::newRow("space") << "com example" << false;
    }

    void test_id()
    {
        QFETCH(QString, id);
        QFETCH(bool, valid);

        auto result = PluginManifest::parse(QString("id = \"%1\"\nname = \"x\"\nversion = \"1\"\napi = 1").arg(id).toUtf8());
        QVERIFY2(bool(result) == valid, qPrintable(result ? id : result.error()));
    }

    void test_entry_data()
    {
        QTest::addColumn<QString>("entry");
        QTest::addColumn<bool>("valid");

        QTest::newRow("file") << "init.lua" << true;
        QTest::newRow("subfolder") << "src/plugin.main.lua" << true;
        QTest::newRow("not lua") << "main.txt" << false;
        QTest::newRow("only extension") << ".lua" << false;
        QTest::newRow("absolute") << "/etc/main.lua" << false;
        QTest::newRow("parent") << "../main.lua" << false;
        QTest::newRow("parent inside") << "src/../../main.lua" << false;
        QTest::newRow("current dir") << "./main.lua" << false;
        QTest::newRow("hidden") << "src/.main.lua" << false;
        QTest::newRow("backslash") << "src\\\\main.lua" << false;
        QTest::newRow("drive") << "C:/main.lua" << false;
        QTest::newRow("trailing slash") << "src/" << false;
    }

    void test_entry()
    {
        QFETCH(QString, entry);
        QFETCH(bool, valid);

        auto result = PluginManifest::parse(manifest(QString("entry = \"%1\"").arg(entry).toUtf8()));
        QVERIFY2(bool(result) == valid, qPrintable(result ? entry : result.error()));
        if (valid) {
            QCOMPARE(result->entry, entry);
        }
    }

    void test_load()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        auto missing = PluginManifest::load(dir.filePath("plugin.toml"));
        QVERIFY(!missing);
        QVERIFY2(missing.error().startsWith("could not open"), qPrintable(missing.error()));

        QFile file(dir.filePath("plugin.toml"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(manifest());
        file.close();

        auto result = PluginManifest::load(file.fileName());
        QVERIFY2(result, qPrintable(result.error_or("")));
        QCOMPARE(result->id, "com.example.autobackup");
    }
};

QTEST_GUILESS_MAIN(PluginManifestTest)

#include "PluginManifest_test.moc"
