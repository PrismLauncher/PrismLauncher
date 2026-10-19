#include <QDir>
#include <QTemporaryDir>
#include <QTest>

#include <plugins/Plugin.h>
#include <plugins/PluginManifest.h>
#include <plugins/lua/LuaState.h>
#include <plugins/lua/PrismApi.h>

class PrismApiTest : public QObject {
    Q_OBJECT

    QTemporaryDir m_dir;

    static PluginManifest manifest()
    {
        PluginManifest manifest;
        manifest.id = "com.example.log";
        manifest.name = "Log";
        manifest.version = "1.0.0";
        manifest.api = 1;
        return manifest;
    }

    static void runOk(LuaState& state, const QByteArray& code)
    {
        auto error = state.run(code, "test");
        QVERIFY2(!error, qPrintable(error.value_or("")));
    }

   private slots:
    void test_log()
    {
        LuaState state;
        QCOMPARE(PrismApi::install(state, manifest(), m_dir.path()), std::nullopt);

        QTest::ignoreMessage(QtInfoMsg, "[com.example.log] hello 1 2.5 true nil");
        QTest::ignoreMessage(QtWarningMsg, "[com.example.log] careful");
        QTest::ignoreMessage(QtCriticalMsg, "[com.example.log] broken");
        QTest::ignoreMessage(QtInfoMsg, "[com.example.log] ");
        runOk(state, R"(
            prism.log.info("hello", 1, 2.5, true, nil)
            prism.log.warn("careful")
            prism.log.error("broken")
            prism.log.info()
        )");
    }

    void test_printIsLogged()
    {
        LuaState state;
        QCOMPARE(PrismApi::install(state, manifest(), m_dir.path()), std::nullopt);
        QTest::ignoreMessage(QtInfoMsg, "[com.example.log] from print");
        runOk(state, "print('from', 'print')");
    }

    void test_tostringMetamethod()
    {
        LuaState state;
        QCOMPARE(PrismApi::install(state, manifest(), m_dir.path()), std::nullopt);
        QTest::ignoreMessage(QtInfoMsg, "[com.example.log] <point 1,2>");
        runOk(state, R"(
            local point = setmetatable({ x = 1, y = 2 }, {
                __tostring = function(p) return "<point " .. p.x .. "," .. p.y .. ">" end,
            })
            prism.log.info(point)
        )");
    }

    void test_tostringErrorIsReported()
    {
        LuaState state;
        QCOMPARE(PrismApi::install(state, manifest(), m_dir.path()), std::nullopt);
        auto error = state.run("prism.log.info(setmetatable({}, { __tostring = function() error('bad') end }))", "test");
        QVERIFY(error);
        QVERIFY2(error->contains("bad"), qPrintable(*error));
    }

    void test_longMessagesAreTruncated()
    {
        LuaState state;
        QCOMPARE(PrismApi::install(state, manifest(), m_dir.path()), std::nullopt);

        const QString expected = "[com.example.log] " + QString(PrismApi::s_maxLogMessageLength, 'x') + " [truncated]";
        QTest::ignoreMessage(QtInfoMsg, qPrintable(expected));
        runOk(state, "prism.log.info(string.rep('x', 1024 * 1024))");

        // Truncation also applies across arguments
        const QString split = "[com.example.log] " + QString(PrismApi::s_maxLogMessageLength - 2, 'a') + " b [truncated]";
        QTest::ignoreMessage(QtInfoMsg, qPrintable(split));
        runOk(state, QString("prism.log.info(string.rep('a', %1), 'bb', 'never')").arg(PrismApi::s_maxLogMessageLength - 2).toUtf8());
    }

    void test_utf8()
    {
        LuaState state;
        QCOMPARE(PrismApi::install(state, manifest(), m_dir.path()), std::nullopt);
        QTest::ignoreMessage(QtInfoMsg, "[com.example.log] ñandú ✓");
        runOk(state, "prism.log.info('ñandú', '✓')");
    }

    void test_pluginLoadsWithApi()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QDir dir(root.path());
        QVERIFY(dir.mkpath("com.example.log"));
        dir.cd("com.example.log");

        QFile toml(dir.filePath("plugin.toml"));
        QVERIFY(toml.open(QIODevice::WriteOnly));
        toml.write("id = \"com.example.log\"\nname = \"Log\"\nversion = \"1\"\napi = 1\n");
        toml.close();
        QFile main(dir.filePath("main.lua"));
        QVERIFY(main.open(QIODevice::WriteOnly));
        main.write("prism.log.info('loaded')");
        main.close();

        Plugin plugin(dir.path());
        QTest::ignoreMessage(QtInfoMsg, "[com.example.log] loaded");
        QVERIFY2(plugin.load(), qPrintable(plugin.error()));
    }
};

QTEST_GUILESS_MAIN(PrismApiTest)

#include "PrismApi_test.moc"
