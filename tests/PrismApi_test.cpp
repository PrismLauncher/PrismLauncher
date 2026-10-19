#include <QDir>
#include <QTemporaryDir>
#include <QTest>

#include <plugins/Plugin.h>
#include <plugins/PluginManifest.h>
#include <plugins/lua/LuaState.h>
#include <plugins/lua/PrismApi.h>

#include <lua.hpp>

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

    void test_events()
    {
        LuaState state;
        QCOMPARE(PrismApi::install(state, manifest(), m_dir.path()), std::nullopt);
        QCOMPARE(PrismApi::emitEvent(state, "launcher.ready"), std::nullopt);

        runOk(state, R"(
            calls = {}
            prism.on("launcher.ready", function() table.insert(calls, "first") end)
            prism.on("launcher.ready", function()
                table.insert(calls, "second")
                -- Registered while dispatching: only runs from the next event on
                prism.on("launcher.ready", function() table.insert(calls, "late") end)
            end)
        )");
        QCOMPARE(PrismApi::emitEvent(state, "launcher.ready"), std::nullopt);
        runOk(state, "assert(table.concat(calls, ',') == 'first,second', table.concat(calls, ','))");
        QCOMPARE(PrismApi::emitEvent(state, "launcher.ready"), std::nullopt);
        runOk(state, "assert(table.concat(calls, ',') == 'first,second,first,second,late', table.concat(calls, ','))");
        QCOMPARE(lua_gettop(state.get()), 0);
    }

    void test_invalidHandlers()
    {
        LuaState state;
        QCOMPARE(PrismApi::install(state, manifest(), m_dir.path()), std::nullopt);
        auto error = state.run("prism.on('no.such.event', function() end)", "test");
        QVERIFY(error && error->contains("unknown event 'no.such.event'"));
        error = state.run("prism.on('launcher.ready', 42)", "test");
        QVERIFY(error && error->contains("function expected"));
    }

    void test_failingHandler()
    {
        LuaState state;
        QCOMPARE(PrismApi::install(state, manifest(), m_dir.path()), std::nullopt);
        runOk(state, R"(
            prism.on("launcher.ready", function() error("boom") end)
            prism.on("launcher.ready", function() reached = true end)
        )");
        auto error = PrismApi::emitEvent(state, "launcher.ready");
        QVERIFY(error);
        QVERIFY2(error->contains("test:2: boom") && error->contains("stack traceback:"), qPrintable(*error));
        runOk(state, "assert(reached == nil)");

        runOk(state, "prism.on('launcher.ready', function() while true do end end)");
        error = PrismApi::emitEvent(state, "launcher.ready");
        QVERIFY(error);
    }

    void test_handlerRunsOutOfInstructions()
    {
        LuaState state;
        QCOMPARE(PrismApi::install(state, manifest(), m_dir.path()), std::nullopt);
        runOk(state, "prism.on('launcher.ready', function() while true do end end)");
        auto error = PrismApi::emitEvent(state, "launcher.ready");
        QVERIFY(error);
        QVERIFY2(error->contains("instruction limit exceeded"), qPrintable(*error));
    }

    void test_eventPermissions()
    {
        LuaState state;
        QCOMPARE(PrismApi::install(state, manifest(), m_dir.path()), std::nullopt);
        auto error = state.run("prism.on('instance.created', function() end)", "test");
        QVERIFY(error);
        QVERIFY2(error->contains("event 'instance.created' requires the 'instances.read' permission"), qPrintable(*error));

        LuaState allowed;
        auto withPermission = manifest();
        withPermission.permissions = { "instances.read" };
        QCOMPARE(PrismApi::install(allowed, withPermission, m_dir.path()), std::nullopt);
        runOk(allowed, R"(
            for _, event in ipairs({ "instance.created", "instance.removed", "instance.status", "launch.step", "launch.finished" }) do
                prism.on(event, function() end)
            end
        )");
    }

    void test_eventPayloadAndFilter()
    {
        LuaState state;
        auto withPermission = manifest();
        withPermission.permissions = { "instances.read" };
        QCOMPARE(PrismApi::install(state, withPermission, m_dir.path()), std::nullopt);
        runOk(state, R"(
            seen = {}
            prism.on("instance.status", function(e)
                table.insert(seen, "all:" .. e.id .. ":" .. tostring(e.running))
                e.id = "changed"
            end)
            prism.on("instance.status", { instance = "b" }, function(e) table.insert(seen, "only b:" .. e.id) end)
        )");
        QCOMPARE(PrismApi::emitEvent(state, "instance.status", { { "id", QByteArray("a") }, { "running", true } }), std::nullopt);
        QCOMPARE(PrismApi::emitEvent(state, "instance.status", { { "id", QByteArray("b") }, { "running", false } }), std::nullopt);
        // Each handler gets its own table, so the change made by the first one is not seen by the second
        runOk(state, "assert(table.concat(seen, ',') == 'all:a:true,all:b:false,only b:b', table.concat(seen, ','))");

        auto error = state.run("prism.on('instance.status', { instance = 3 }, function() end)", "test");
        QVERIFY(error && error->contains("'instance' must be an instance id"));
        error = state.run("prism.on('instance.status', {})", "test");
        QVERIFY(error && error->contains("function expected"));
    }

    void test_instances()
    {
        struct FakeSource : PrismApi::InstanceSource {
            QList<PrismApi::InstanceInfo> instances() const override
            {
                return { { "a", "Alpha", "Modded", true, 1700000000, 3600 }, { "b", "Beta ñ", "", false, 0, 0 } };
            }
        } source;

        auto withPermission = manifest();
        withPermission.permissions = { "instances.read" };
        LuaState state;
        QCOMPARE(PrismApi::install(state, withPermission, m_dir.path(), &source), std::nullopt);
        runOk(state, R"(
            local list = prism.instances.list()
            assert(#list == 2)
            assert(list[1].id == "a" and list[1].name == "Alpha" and list[1].group == "Modded")
            assert(list[1].running == true and list[1].lastLaunch == 1700000000 and list[1].playTime == 3600)
            assert(list[2].name == "Beta ñ" and list[2].running == false)

            local b = prism.instances.get("b")
            assert(b and b.id == "b")
            assert(prism.instances.get("missing") == nil)
            -- Results are copies
            list[1].name = "changed"
            assert(prism.instances.get("a").name == "Alpha")
        )");
        QVERIFY(source.scratch.isEmpty());

        // Without a source there are no instances
        LuaState empty;
        QCOMPARE(PrismApi::install(empty, withPermission, m_dir.path()), std::nullopt);
        runOk(empty, "assert(#prism.instances.list() == 0 and prism.instances.get('a') == nil)");

        // Without the permission
        LuaState denied;
        QCOMPARE(PrismApi::install(denied, manifest(), m_dir.path(), &source), std::nullopt);
        auto error = denied.run("prism.instances.list()", "test");
        QVERIFY(error && error->contains("prism.instances requires the 'instances.read' permission"));
        error = denied.run("prism.instances.get('a')", "test");
        QVERIFY(error && error->contains("requires the 'instances.read' permission"));
    }

    void test_uiActions()
    {
        auto withPermission = manifest();
        withPermission.permissions = { "ui" };
        LuaState state;
        QCOMPARE(PrismApi::install(state, withPermission, m_dir.path()), std::nullopt);
        runOk(state, R"(
            prism.ui.addAction{ text = "Back up", scope = "instance", run = function(ctx) last = "instance:" .. ctx.instance.name end }
            prism.ui.addAction{ text = "Say hi", scope = "launcher", run = function(ctx) last = "launcher:" .. tostring(ctx.instance) end }
            prism.ui.addAction{ text = "Broken", scope = "launcher", run = function() error("nope") end }
        )");

        QList<PrismApi::UiActionInfo> actions;
        QCOMPARE(PrismApi::uiActions(state, actions), std::nullopt);
        QCOMPARE(actions.size(), 3);
        QCOMPARE(actions[0].text, "Back up");
        QCOMPARE(actions[0].scope, PrismApi::ActionScope::Instance);
        QCOMPARE(actions[1].scope, PrismApi::ActionScope::Launcher);

        const PrismApi::InstanceInfo instance{ "a", "Alpha", "", false, 0, 0 };
        QCOMPARE(PrismApi::runUiAction(state, actions[0].index, &instance), std::nullopt);
        runOk(state, "assert(last == 'instance:Alpha', last)");
        QCOMPARE(PrismApi::runUiAction(state, actions[1].index, nullptr), std::nullopt);
        runOk(state, "assert(last == 'launcher:nil', last)");

        auto error = PrismApi::runUiAction(state, actions[2].index, nullptr);
        QVERIFY(error && error->contains("nope") && error->contains("stack traceback:"));
        QVERIFY(PrismApi::runUiAction(state, 99, nullptr));
    }

    void test_uiActionErrors()
    {
        LuaState denied;
        QCOMPARE(PrismApi::install(denied, manifest(), m_dir.path()), std::nullopt);
        auto error = denied.run("prism.ui.addAction{ text = 'x', scope = 'launcher', run = function() end }", "test");
        QVERIFY(error && error->contains("prism.ui.addAction requires the 'ui' permission"));

        auto withPermission = manifest();
        withPermission.permissions = { "ui" };
        LuaState state;
        QCOMPARE(PrismApi::install(state, withPermission, m_dir.path()), std::nullopt);
        runOk(state, R"(
            local function check(spec, expected)
                local ok, err = pcall(prism.ui.addAction, spec)
                assert(not ok and err:find(expected, 1, true), tostring(err))
            end
            check({ scope = "launcher", run = function() end }, "'text' must be a non-empty string")
            check({ text = "", scope = "launcher", run = function() end }, "'text' must be a non-empty string")
            check({ text = string.rep("x", 101), scope = "launcher", run = function() end }, "'text' is longer than 100 bytes")
            check({ text = "x", scope = "window", run = function() end }, "'scope' must be")
            check({ text = "x", scope = "launcher" }, "'run' must be a function")
            for i = 1, 16 do prism.ui.addAction{ text = "a" .. i, scope = "launcher", run = function() end } end
            check({ text = "x", scope = "launcher", run = function() end }, "too many actions, the maximum is 16")
        )");
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
