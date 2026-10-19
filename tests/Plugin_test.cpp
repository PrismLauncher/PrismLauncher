#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QTemporaryDir>
#include <QTest>

#include <plugins/Plugin.h>
#include <plugins/lua/LuaState.h>

#include <lua.hpp>

class PluginTest : public QObject {
    Q_OBJECT

    QTemporaryDir m_root;

    // Creates <root>/<folder> with the given files and returns its path
    QString makePlugin(const QString& folder, const QMap<QString, QByteArray>& files)
    {
        QDir dir(m_root.path());
        dir.mkpath(folder);
        dir.cd(folder);
        for (auto it = files.begin(); it != files.end(); ++it) {
            dir.mkpath(QFileInfo(dir.filePath(it.key())).path());
            QFile file(dir.filePath(it.key()));
            if (!file.open(QIODevice::WriteOnly)) {
                qFatal("could not write %s", qPrintable(file.fileName()));
            }
            file.write(it.value());
        }
        return dir.path();
    }

    static QByteArray manifest(const QByteArray& id, const QByteArray& extra = {})
    {
        return "id = \"" + id + "\"\nname = \"Test\"\nversion = \"1.0.0\"\napi = 1\n" + extra;
    }

    static QString globalString(Plugin& plugin, const char* name)
    {
        lua_State* L = plugin.lua()->get();
        lua_getglobal(L, name);
        QString value = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        return value;
    }

   private slots:
    void initTestCase() { QVERIFY(m_root.isValid()); }

    void test_loadAndUnload()
    {
        Plugin plugin(
            makePlugin("com.example.ok", { { "plugin.toml", manifest("com.example.ok") }, { "main.lua", "greeting = 'hello'" } }));
        QCOMPARE(plugin.id(), "com.example.ok");
        QVERIFY(plugin.manifest());
        QCOMPARE(plugin.state(), Plugin::State::Disabled);
        QVERIFY(!plugin.lua());

        QVERIFY(plugin.load());
        QCOMPARE(plugin.state(), Plugin::State::Loaded);
        QVERIFY(plugin.error().isEmpty());
        QVERIFY(plugin.lua());
        QCOMPARE(globalString(plugin, "greeting"), "hello");

        // Loading again does not rerun the entry
        QCOMPARE(plugin.lua()->run("greeting = 'changed'", "test"), std::nullopt);
        QVERIFY(plugin.load());
        QCOMPARE(globalString(plugin, "greeting"), "changed");

        plugin.unload();
        QCOMPARE(plugin.state(), Plugin::State::Disabled);
        QVERIFY(!plugin.lua());

        // Reloading starts from a fresh state
        QVERIFY(plugin.load());
        QCOMPARE(globalString(plugin, "greeting"), "hello");
    }

    void test_customEntry()
    {
        Plugin plugin(makePlugin(
            "custom", { { "plugin.toml", manifest("custom", "entry = \"src/init.lua\"") }, { "src/init.lua", "loaded = 'yes'" } }));
        QVERIFY2(plugin.load(), qPrintable(plugin.error()));
        QCOMPARE(globalString(plugin, "loaded"), "yes");
    }

    void test_scriptErrors_data()
    {
        QTest::addColumn<QByteArray>("code");
        QTest::addColumn<QString>("error");

        QTest::newRow("runtime") << QByteArray("error('boom')") << "main.lua:1: boom";
        QTest::newRow("syntax") << QByteArray("this is not lua") << "main.lua:1:";
        QTest::newRow("infinite loop") << QByteArray("while true do end") << "instruction limit exceeded";
        QTest::newRow("sandbox") << QByteArray("os.exit()") << "attempt to index a nil value (global 'os')";
    }

    void test_scriptErrors()
    {
        QFETCH(QByteArray, code);
        QFETCH(QString, error);

        const QString folder = QString("errors.%1").arg(QTest::currentDataTag()).replace(' ', '-');
        Plugin plugin(makePlugin(folder, { { "plugin.toml", manifest(folder.toUtf8()) }, { "main.lua", code } }));
        QVERIFY(!plugin.load());
        QCOMPARE(plugin.state(), Plugin::State::Error);
        QVERIFY2(plugin.error().contains(error), qPrintable(plugin.error()));
        QVERIFY(!plugin.lua());

        // Disabling an errored plugin clears the error
        plugin.unload();
        QCOMPARE(plugin.state(), Plugin::State::Disabled);
        QVERIFY(plugin.error().isEmpty());
    }

    void test_missingEntry()
    {
        Plugin plugin(makePlugin("missing", { { "plugin.toml", manifest("missing") } }));
        QVERIFY(!plugin.load());
        QCOMPARE(plugin.state(), Plugin::State::Error);
        QVERIFY2(plugin.error().startsWith("could not open main.lua"), qPrintable(plugin.error()));
    }

    void test_entryTooLarge()
    {
        const QByteArray code = "-- " + QByteArray(Plugin::s_maxEntrySize, 'x');
        Plugin plugin(makePlugin("large", { { "plugin.toml", manifest("large") }, { "main.lua", code } }));
        QVERIFY(!plugin.load());
        QVERIFY2(plugin.error().contains("is larger than"), qPrintable(plugin.error()));
    }

    void test_invalidManifest()
    {
        Plugin plugin(makePlugin("broken", { { "plugin.toml", "id = " }, { "main.lua", "" } }));
        QVERIFY(!plugin.manifest());
        QCOMPARE(plugin.id(), "broken");
        QCOMPARE(plugin.state(), Plugin::State::Error);
        QVERIFY2(plugin.error().startsWith("invalid TOML"), qPrintable(plugin.error()));

        QVERIFY(!plugin.load());
        QCOMPARE(plugin.state(), Plugin::State::Error);
    }

    void test_missingManifest()
    {
        Plugin plugin(makePlugin("nomanifest", { { "main.lua", "" } }));
        QCOMPARE(plugin.id(), "nomanifest");
        QCOMPARE(plugin.state(), Plugin::State::Error);
        QVERIFY2(plugin.error().startsWith("could not open"), qPrintable(plugin.error()));
    }

    void test_idMustMatchFolder()
    {
        Plugin plugin(makePlugin("folder", { { "plugin.toml", manifest("com.example.other") }, { "main.lua", "" } }));
        QCOMPARE(plugin.id(), "folder");
        QCOMPARE(plugin.state(), Plugin::State::Error);
        QVERIFY2(plugin.error().contains("does not match its folder name 'folder'"), qPrintable(plugin.error()));
    }

    void test_unrestricted()
    {
        const QString outside = m_root.filePath("outside.txt");
        Plugin plugin(makePlugin("free", { { "plugin.toml", manifest("free", "permissions = [\"unrestricted\"]") },
                                           { "helpers.lua", "return { double = function(x) return x * 2 end }" },
                                           { "main.lua", QString("local helpers = require('helpers')\n"
                                                                 "doubled = helpers.double(21)\n"
                                                                 "local f = assert(io.open('%1', 'w'))\n"
                                                                 "f:write('written by a plugin')\n"
                                                                 "f:close()\n"
                                                                 "clock = type(os.time())\n"
                                                                 "hasDebug = tostring(debug ~= nil)\n")
                                                             .arg(outside)
                                                             .toUtf8() } }));
        QVERIFY2(plugin.load(), qPrintable(plugin.error()));
        QCOMPARE(globalString(plugin, "doubled"), "42");
        QCOMPARE(globalString(plugin, "clock"), "number");
        QCOMPARE(globalString(plugin, "hasDebug"), "false");
        QFile file(outside);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), "written by a plugin");

        // The watchdog still applies
        QCOMPARE(plugin.lua()->run("while true do end", "loop").value_or("").contains("instruction limit exceeded"), true);
    }

    void test_sandboxedByDefault()
    {
        Plugin plugin(
            makePlugin("boxed", { { "plugin.toml", manifest("boxed") },
                                  { "main.lua", "names = tostring(io) .. tostring(os) .. tostring(package) .. type(require)" } }));
        QVERIFY2(plugin.load(), qPrintable(plugin.error()));
        // require only loads modules from the plugin's folder
        QCOMPARE(globalString(plugin, "names"), "nilnilnilfunction");
    }

    void test_require()
    {
        Plugin plugin(makePlugin("modules", { { "plugin.toml", manifest("modules") },
                                              { "util/strings.lua", "loads = (loads or 0) + 1 return { shout = string.upper }" },
                                              { "util/init.lua", "return { name = ... }" },
                                              { "noresult.lua", "x = 1" },
                                              { "broken.lua", "error(\"broken module\")" },
                                              { "loop.lua", "require(\"loop\")" },
                                              { "main.lua",
                                                "local s = require(\"util.strings\")\n"
                                                "assert(require(\"util.strings\") == s)\n"
                                                "shouted = s.shout(\"hi\")\n"
                                                "utilName = require(\"util\").name\n"
                                                "noresult = tostring(require(\"noresult\"))\n"
                                                "local function message(name) local ok, err = pcall(require, name) return err end\n"
                                                "missing = message(\"missing\")\n"
                                                "escape = message(\"..secret\")\n"
                                                "broken = message(\"broken\")\n"
                                                "loop = message(\"loop\")\n" } }));
        QVERIFY2(plugin.load(), qPrintable(plugin.error()));
        QCOMPARE(globalString(plugin, "shouted"), "HI");
        QCOMPARE(globalString(plugin, "loads"), "1");
        QCOMPARE(globalString(plugin, "utilName"), "util");
        QCOMPARE(globalString(plugin, "noresult"), "true");
        QVERIFY(globalString(plugin, "missing").contains("module 'missing' not found"));
        QVERIFY(globalString(plugin, "escape").contains("invalid module name '..secret'"));
        QVERIFY(globalString(plugin, "broken").contains("broken module"));
        QVERIFY(globalString(plugin, "loop").contains("module 'loop' requires itself"));
    }

    void test_trailingSlash()
    {
        Plugin plugin(makePlugin("slash", { { "plugin.toml", manifest("slash") }, { "main.lua", "" } }) + "/");
        QCOMPARE(plugin.id(), "slash");
        QVERIFY2(plugin.load(), qPrintable(plugin.error()));
    }
};

QTEST_GUILESS_MAIN(PluginTest)

#include "Plugin_test.moc"
