#include <QDir>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <launch/LaunchStep.h>
#include <launch/LaunchTask.h>
#include <launch/LogModel.h>
#include <plugins/Plugin.h>
#include <plugins/PluginManager.h>
#include <plugins/lua/LuaState.h>
#include <settings/INISettingsObject.h>
#include <settings/Setting.h>

#include <memory>

namespace {

class DummyStep : public LaunchStep {
    Q_OBJECT
   public:
    explicit DummyStep(LaunchTask* parent) : LaunchStep(parent) {}

   protected:
    void executeTask() override { emitSucceeded(); }
};

// create() needs a real instance; the steps can be tested without one
class TestLaunchTask : public LaunchTask {
   public:
    // The log model would otherwise be created from the instance's settings
    TestLaunchTask() : LaunchTask(nullptr) { m_logModel.reset(new LogModel()); }

    void addPhase(const QString& id) { appendStep(makeShared<DummyStep>(this), id); }
    QList<shared_qobject_ptr<LaunchStep>> steps() const { return m_steps; }
};

}  // namespace

class LuaLaunchStepTest : public QObject {
    Q_OBJECT

    std::unique_ptr<QTemporaryDir> m_root;
    std::unique_ptr<INISettingsObject> m_settings;
    std::unique_ptr<PluginManager> m_manager;

    void addPlugin(const QString& id, const QByteArray& code, const QByteArray& permissions = "[\"launch.hooks\"]")
    {
        QDir dir(m_root->filePath("plugins"));
        QVERIFY(dir.mkpath(id));
        QFile toml(dir.filePath(id + "/plugin.toml"));
        QVERIFY(toml.open(QIODevice::WriteOnly));
        toml.write("id = \"" + id.toUtf8() + "\"\nname = \"Plugin " + id.toUtf8() +
                   "\"\nversion = \"1\"\napi = 1\npermissions = " + permissions + "\n");
        QFile main(dir.filePath(id + "/main.lua"));
        QVERIFY(main.open(QIODevice::WriteOnly));
        main.write(code);
    }

    void startManager(const QStringList& enabled)
    {
        auto setting = m_settings->registerSetting("EnabledPlugins", QStringList());
        setting->set(enabled);
        m_manager = std::make_unique<PluginManager>(m_root->filePath("plugins"), setting);
        m_manager->discover();
    }

    static std::unique_ptr<TestLaunchTask> phases()
    {
        auto task = std::make_unique<TestLaunchTask>();
        task->addPhase("java");
        task->addPhase("java");
        task->addPhase("launch");
        return task;
    }

    // Runs the step synchronously and returns whether it succeeded, collecting its log lines
    static bool runStep(LaunchTask* task, const shared_qobject_ptr<LaunchStep>& step, QStringList* lines = nullptr)
    {
        // Detach it from the test task, which has no instance or log to drive a real launch
        step->disconnect(task);
        QObject context;
        if (lines) {
            connect(step.get(), &LaunchStep::logLine, &context, [lines](const QString& line, MessageLevel) { lines->append(line); });
        }
        step->start();
        return step->wasSuccessful();
    }

   private slots:
    void init()
    {
        m_root = std::make_unique<QTemporaryDir>();
        QVERIFY(m_root->isValid());
        m_settings = std::make_unique<INISettingsObject>(m_root->filePath("settings.cfg"));
    }

    void cleanup()
    {
        m_manager.reset();
        m_settings.reset();
        m_root.reset();
    }

    void test_stepsAreInsertedInOrder()
    {
        addPlugin("com.example.a", R"(
            prism.launch.addStep{ before = "launch", name = "a1", run = function() end }
            prism.launch.addStep{ after = "java", name = "a2", run = function() end }
        )");
        addPlugin("com.example.b", R"(
            prism.launch.addStep{ before = "launch", name = "b1", run = function() end }
            prism.launch.addStep{ after = "java", run = function() end }
            prism.launch.addStep{ before = "post-exit-command", name = "missing phase", run = function() end }
        )");
        startManager({ "com.example.a", "com.example.b" });

        auto task = phases();
        QTest::ignoreMessage(QtInfoMsg,
                             QRegularExpression("skipping launch step \"missing phase\": this launch has no 'post-exit-command'"));
        m_manager->contributeLaunchSteps(task.get());
        QCOMPARE(task->stepIds(), QStringList({ "java", "java", "plugin:com.example.a", "plugin:com.example.b", "plugin:com.example.a",
                                                "plugin:com.example.b", "launch" }));

        // Registration order is kept within each phase, and the default name is the plugin's
        QStringList names;
        for (const auto& step : task->steps()) {
            if (!step->id().startsWith("plugin:")) {
                continue;
            }
            QStringList lines;
            QVERIFY(runStep(task.get(), step, &lines));
            names.append(lines.first());
        }
        QCOMPARE(names,
                 QStringList({ "Running \"a2\" from plugin com.example.a", "Running \"Plugin com.example.b\" from plugin com.example.b",
                               "Running \"a1\" from plugin com.example.a", "Running \"b1\" from plugin com.example.b" }));
    }

    void test_contextAndLog()
    {
        addPlugin("com.example.ctx", R"(
            prism.launch.addStep{ before = "launch", name = "ctx", run = function(ctx)
                assert(type(ctx.instance) == "table")
                ctx.log("hello from the step")
                saved = ctx
            end }
            prism.launch.addStep{ before = "launch", name = "stale", run = function()
                saved.log("too late")
            end }
        )");
        startManager({ "com.example.ctx" });
        auto task = phases();
        m_manager->contributeLaunchSteps(task.get());

        QStringList lines;
        QVERIFY(runStep(task.get(), task->steps().at(2), &lines));
        QVERIFY2(lines.contains("hello from the step"), qPrintable(lines.join('\n')));

        // A ctx kept after its step finished cannot be used, but the launch goes on
        lines.clear();
        QVERIFY(runStep(task.get(), task->steps().at(3), &lines));
        QVERIFY2(lines.last().contains("this launch step has already finished"), qPrintable(lines.join('\n')));
    }

    void test_errorsContinueAbortStops()
    {
        addPlugin("com.example.fail", R"(
            prism.launch.addStep{ before = "launch", name = "broken", run = function() error("boom") end }
            prism.launch.addStep{ before = "launch", name = "loop", run = function() while true do end end }
            prism.launch.addStep{ before = "launch", name = "abort", run = function(ctx)
                -- Catching the abort does not undo it
                pcall(ctx.abort, "no backups left")
            end }
        )");
        startManager({ "com.example.fail" });
        auto task = phases();
        m_manager->contributeLaunchSteps(task.get());

        QStringList lines;
        QVERIFY(runStep(task.get(), task->steps().at(2), &lines));
        QVERIFY2(lines.last().contains("\"broken\" from plugin com.example.fail failed, continuing") && lines.last().contains("boom"),
                 qPrintable(lines.join('\n')));

        lines.clear();
        QVERIFY(runStep(task.get(), task->steps().at(3), &lines));
        QVERIFY2(lines.last().contains("instruction limit exceeded"), qPrintable(lines.join('\n')));

        auto abort = task->steps().at(4);
        QTest::ignoreMessage(QtCriticalMsg, QRegularExpression("aborted the launch: no backups left"));
        QVERIFY(!runStep(task.get(), abort));
        QCOMPARE(abort->failReason(), "Plugin com.example.fail aborted the launch: no backups left");
    }

    void test_unloadedPluginIsSkipped()
    {
        addPlugin("com.example.gone", "prism.launch.addStep{ before = 'launch', name = 'gone', run = function() end }");
        startManager({ "com.example.gone" });
        auto task = phases();
        m_manager->contributeLaunchSteps(task.get());
        auto step = task->steps().at(2);

        // Reloaded: the prepared step belongs to the previous run
        m_manager->setEnabled("com.example.gone", false);
        m_manager->setEnabled("com.example.gone", true);
        QStringList lines;
        QVERIFY(runStep(task.get(), step, &lines));
        QVERIFY2(lines.last().contains("was unloaded or reloaded"), qPrintable(lines.join('\n')));
    }

    void test_launchEvents()
    {
        addPlugin("com.example.watch", R"(
            prism.on("launch.step", function(e) prism.log.info("step " .. e.step) end)
            prism.on("launch.finished", function(e) prism.log.info("finished " .. tostring(e.success) .. " '" .. e.reason .. "'") end)
        )",
                  "[\"instances.read\"]");
        startManager({ "com.example.watch" });
        auto task = phases();
        m_manager->watchLaunch(task.get());

        QTest::ignoreMessage(QtInfoMsg, "[com.example.watch] step java");
        emit task->stepStarted("java");
        QTest::ignoreMessage(QtInfoMsg, "[com.example.watch] finished false ''");
        emit task->finished();
    }

    void test_gameLog()
    {
        addPlugin("com.example.log", R"(
            lines, skipped = {}, 0
            prism.on("game.log", function(e)
                table.insert(lines, e.level .. ":" .. e.line)
                skipped = skipped + e.skipped
            end)
        )",
                  "[\"game.log\"]");
        startManager({ "com.example.log" });
        auto task = phases();
        m_manager->watchLaunch(task.get());
        task->setCensorFilter({ { "s3cr3t-token", "<ACCESS TOKEN>" } });

        task->onLogLine("[12:00:00] [main/INFO]: hello with s3cr3t-token", MessageLevel::Info);
        task->onLogLine("launcher's own line", MessageLevel::Launcher);
        task->onLogLine("oops", MessageLevel::Error);
        LuaState* lua = m_manager->plugin("com.example.log")->lua();
        QCOMPARE(lua->run(R"(
            assert(#lines == 2, #lines)
            assert(lines[1] == "info:[12:00:00] [main/INFO]: hello with <ACCESS TOKEN>", lines[1])
            assert(lines[2] == "error:oops", lines[2])
        )",
                          "check"),
                 std::nullopt);

        // At most s_maxGameLogLinesPerSecond lines per second; the next delivered line reports how many were dropped
        for (int i = 0; i < PluginManager::s_maxGameLogLinesPerSecond + 50; i++) {
            task->onLogLine(QString("spam %1").arg(i), MessageLevel::Info);
        }
        QCOMPARE(lua->run("assert(#lines <= 2 + 200, #lines) delivered = #lines", "check"), std::nullopt);
        QTest::qWait(1100);
        task->onLogLine("after the burst", MessageLevel::Info);
        QCOMPARE(lua->run("assert(lines[#lines] == 'info:after the burst') assert(skipped == 2 + 250 - delivered, skipped)", "check"),
                 std::nullopt);
    }

    void test_gameLogNeedsPermission()
    {
        addPlugin("com.example.nolog", "prism.on('game.log', function() end)", "[\"instances.read\"]");
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("^Failed to load plugin \"com.example.nolog\""));
        startManager({ "com.example.nolog" });
        QVERIFY2(m_manager->plugin("com.example.nolog")->error().contains("event 'game.log' requires the 'game.log' permission"),
                 qPrintable(m_manager->plugin("com.example.nolog")->error()));
    }

    void test_registrationErrors()
    {
        addPlugin("com.example.noperm", "prism.launch.addStep{ before = 'launch', run = function() end }", "[]");
        addPlugin("com.example.bad", R"(
            local function check(spec, expected)
                local ok, err = pcall(prism.launch.addStep, spec)
                assert(not ok and err:find(expected, 1, true), tostring(err))
            end
            check({ run = function() end }, "set either 'before' or 'after'")
            check({ before = "launch", after = "java", run = function() end }, "set either 'before' or 'after'")
            check({ before = "nope", run = function() end }, "unknown launch phase 'nope'")
            check({ before = "launch" }, "'run' must be a function")
            check({ before = "launch", name = 3, run = function() end }, "'name' must be a string")
            for i = 1, 16 do prism.launch.addStep{ before = "launch", run = function() end } end
            check({ before = "launch", run = function() end }, "too many launch steps, the maximum is 16")
        )");
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("^Failed to load plugin \"com.example.noperm\""));
        startManager({ "com.example.noperm", "com.example.bad" });
        QVERIFY2(m_manager->plugin("com.example.noperm")->error().contains("requires the 'launch.hooks' permission"),
                 qPrintable(m_manager->plugin("com.example.noperm")->error()));
        QCOMPARE(m_manager->plugin("com.example.bad")->state(), Plugin::State::Loaded);
    }
};

QTEST_GUILESS_MAIN(LuaLaunchStepTest)

#include "LuaLaunchStep_test.moc"
