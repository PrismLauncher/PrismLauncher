#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <plugins/PluginManifest.h>
#include <plugins/lua/LuaState.h>
#include <plugins/lua/PrismApi.h>
#include <plugins/lua/SettingsApi.h>

#include <memory>

class SettingsApiTest : public QObject {
    Q_OBJECT

    std::unique_ptr<QTemporaryDir> m_dir;

    QString settingsPath() const { return m_dir->filePath("settings.ini"); }

    std::unique_ptr<LuaState> newState()
    {
        PluginManifest manifest;
        manifest.id = "com.example.settings";
        auto state = std::make_unique<LuaState>();
        auto error = PrismApi::install(*state, manifest, m_dir->path());
        if (error) {
            qFatal("install failed: %s", qPrintable(*error));
        }
        return state;
    }

    static void runOk(LuaState& state, const QByteArray& code)
    {
        auto error = state.run(code, "test");
        QVERIFY2(!error, qPrintable(error.value_or("")));
    }

    static void expectError(LuaState& state, const QByteArray& code, const QString& expected)
    {
        auto error = state.run(code, "test");
        QVERIFY2(error, "the code should have failed");
        QVERIFY2(error->contains(expected), qPrintable(*error));
    }

    static constexpr const char* s_defaults = R"(
        settings = prism.settings{ enabled = true, interval = 10, ratio = 0.5, name = "backup" }
    )";

   private slots:
    void init()
    {
        m_dir = std::make_unique<QTemporaryDir>();
        QVERIFY(m_dir->isValid());
    }

    void cleanup() { m_dir.reset(); }

    void test_defaults()
    {
        auto state = newState();
        runOk(*state, s_defaults);
        runOk(*state, R"(
            assert(settings.enabled == true)
            assert(settings.interval == 10 and math.type(settings.interval) == "integer")
            assert(settings.ratio == 0.5)
            assert(settings.name == "backup")
            assert(getmetatable(settings) == false)
        )");
        // Nothing was written
        QVERIFY(!QFile::exists(settingsPath()));
    }

    void test_persistsAcrossStates()
    {
        {
            auto state = newState();
            runOk(*state, s_defaults);
            runOk(*state, R"(
                settings.enabled = false
                settings.interval = 30.0
                settings.ratio = 2
                settings.name = "ñandú = [x]; y"
                assert(settings.interval == 30 and math.type(settings.interval) == "integer")
            )");
        }
        // Closing the state flushed the changes
        QVERIFY(QFile::exists(settingsPath()));

        auto state = newState();
        runOk(*state, s_defaults);
        runOk(*state, R"(
            assert(settings.enabled == false)
            assert(settings.interval == 30 and math.type(settings.interval) == "integer")
            assert(settings.ratio == 2)
            assert(settings.name == "ñandú = [x]; y", settings.name)
        )");
    }

    void test_flushedAfterCall()
    {
        auto state = newState();
        runOk(*state, s_defaults);
        runOk(*state, "for i = 1, 1000 do settings.interval = i end");
        QVERIFY(!QFile::exists(settingsPath()));

        // The write happens once control returns to the event loop
        QCoreApplication::processEvents();
        QVERIFY(QFile::exists(settingsPath()));
        QFile file(settingsPath());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QVERIFY(file.readAll().contains("interval=1000"));
    }

    void test_resetWithNil()
    {
        {
            auto state = newState();
            runOk(*state, s_defaults);
            runOk(*state, "settings.name = 'changed'; settings.name = nil; assert(settings.name == 'backup')");
        }
        auto state = newState();
        runOk(*state, s_defaults);
        runOk(*state, "assert(settings.name == 'backup')");
    }

    void test_typeErrors()
    {
        auto state = newState();
        runOk(*state, s_defaults);
        expectError(*state, "settings.enabled = 1", "setting 'enabled' must be a boolean, got number");
        expectError(*state, "settings.interval = '5'", "setting 'interval' must be a number, got string");
        expectError(*state, "settings.interval = 1.5", "setting 'interval' must be an integer");
        expectError(*state, "settings.name = {}", "setting 'name' must be a string, got table");
        expectError(*state, "settings.name = string.rep('x', 64 * 1024 + 1)", "is longer than");
        expectError(*state, "settings.missing = 1", "unknown setting 'missing'");
        expectError(*state, "local x = settings.missing", "unknown setting 'missing'");
        expectError(*state, "local x = settings[1]", "setting names must be strings, got number");
        // Failed assignments change nothing
        runOk(*state, "assert(settings.enabled == true and settings.interval == 10 and settings.name == 'backup')");
    }

    void test_invalidDefaults()
    {
        auto state = newState();
        expectError(*state, "prism.settings()", "table expected");
        expectError(*state, "prism.settings{ [1] = true }", "setting names must be strings, got number");
        expectError(*state, "prism.settings{ ['a/b'] = true }", "invalid setting name 'a/b'");
        expectError(*state, "prism.settings{ ConfigVersion = '1' }", "invalid setting name 'ConfigVersion'");
        expectError(*state, "prism.settings{ list = {} }", "must be a boolean, number or string");
        expectError(*state, "prism.settings{ big = string.rep('x', 64 * 1024 + 1) }", "is longer than");
        expectError(*state, "local t = {} for i = 1, 257 do t['k' .. i] = i end prism.settings(t)",
                    "too many settings, the maximum is 256");
    }

    void test_onlyOnce()
    {
        auto state = newState();
        runOk(*state, s_defaults);
        expectError(*state, "prism.settings{ other = 1 }", "prism.settings can only be called once");
    }

    void test_defaultsAreCopied()
    {
        auto state = newState();
        runOk(*state, R"(
            local defaults = { level = 1 }
            local settings = prism.settings(defaults)
            defaults.level = "changed"
            defaults.extra = true
            assert(settings.level == 1)
            settings.level = 2
            assert(not pcall(function() settings.extra = true end))
        )");
    }

    void test_corruptedFileFallsBackToDefaults()
    {
        QFile file(settingsPath());
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("[General]\nConfigVersion=1.3\nenabled=maybe\ninterval=lots\nratio=x\nname=kept\n");
        file.close();

        auto state = newState();
        runOk(*state, s_defaults);
        runOk(*state, R"(
            assert(settings.enabled == true)
            assert(settings.interval == 10)
            assert(settings.ratio == 0.5)
            assert(settings.name == "kept")
        )");
    }

    void test_errorsInLoopsAreSafe()
    {
        // Errors raised by the API are caught many times without breaking the state
        auto state = newState();
        runOk(*state, s_defaults);
        runOk(*state, R"(
            for i = 1, 10000 do
                assert(not pcall(function() settings.interval = "bad" end))
                assert(not pcall(function() return settings.nope end))
            end
            settings.interval = 42
            assert(settings.interval == 42)
        )");
    }
};

QTEST_GUILESS_MAIN(SettingsApiTest)

#include "SettingsApi_test.moc"
