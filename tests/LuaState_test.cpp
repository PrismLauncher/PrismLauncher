#include <QRegularExpression>
#include <QTest>

#include <plugins/lua/LuaState.h>

#include <lua.hpp>

class LuaStateTest : public QObject {
    Q_OBJECT

    static QString globalString(LuaState& state, const char* name)
    {
        lua_getglobal(state.get(), name);
        QString value = QString::fromUtf8(lua_tostring(state.get(), -1));
        lua_pop(state.get(), 1);
        return value;
    }

   private slots:
    void test_runsCode()
    {
        LuaState state;
        QVERIFY(state.isValid());
        QCOMPARE(state.run("result = string.upper('ok') .. math.floor(1.5)", "test"), std::nullopt);
        QCOMPARE(globalString(state, "result"), "OK1");
        QCOMPARE(lua_gettop(state.get()), 0);
    }

    void test_syntaxError()
    {
        LuaState state;
        auto error = state.run("this is not lua", "broken.lua");
        QVERIFY(error);
        QVERIFY2(error->startsWith("broken.lua:1:"), qPrintable(*error));
        QCOMPARE(lua_gettop(state.get()), 0);
    }

    void test_runtimeError()
    {
        LuaState state;
        auto error = state.run("local function f() error('boom') end\nf()", "main.lua");
        QVERIFY(error);
        QVERIFY2(error->startsWith("main.lua:1: boom"), qPrintable(*error));
        QVERIFY2(error->contains("stack traceback:"), qPrintable(*error));
        QCOMPARE(lua_gettop(state.get()), 0);
    }

    void test_nonStringError()
    {
        LuaState state;
        auto error = state.run("error({})", "main.lua");
        QVERIFY(error);
        QVERIFY2(error->startsWith("(error object is a table value)"), qPrintable(*error));
    }

    void test_safeLibrariesOnly()
    {
        LuaState state;
        auto error = state.run(R"(
            for _, name in ipairs({ "io", "os", "debug", "package", "require", "dofile", "loadfile" }) do
                assert(_G[name] == nil, name .. " should not be available")
            end
            for _, name in ipairs({ "string", "table", "math", "utf8", "coroutine", "load", "pcall" }) do
                assert(_G[name] ~= nil, name .. " should be available")
            end
        )",
                               "libs");
        QVERIFY2(!error, qPrintable(error.value_or("")));
    }

    void test_loadRejectsBytecode()
    {
        LuaState state;
        auto error = state.run(R"(
            assert(load("return 1 + 1")() == 2)
            assert(load("return x", "chunk", "t", { x = 3 })() == 3)

            local bytecode = string.dump(function() return 1 end)
            assert(load(bytecode) == nil)
            assert(load(bytecode, "chunk", "b") == nil)
            assert(load(bytecode, "chunk", "bt", {}) == nil)

            dumped = bytecode
        )",
                               "load");
        QVERIFY2(!error, qPrintable(error.value_or("")));

        lua_getglobal(state.get(), "dumped");
        size_t size = 0;
        const char* data = lua_tolstring(state.get(), -1, &size);
        const QByteArray bytecode(data, static_cast<qsizetype>(size));
        lua_pop(state.get(), 1);

        auto runError = state.run(bytecode, "bytecode");
        QVERIFY(runError);
        QVERIFY2(runError->contains("binary"), qPrintable(*runError));
    }

    void test_statesAreIsolated()
    {
        LuaState first;
        LuaState second;
        QCOMPARE(first.run("shared = 'first'", "first"), std::nullopt);
        QCOMPARE(second.run("assert(shared == nil)", "second"), std::nullopt);
    }

    void test_memoryIsTracked()
    {
        LuaState state;
        QVERIFY(state.isValid());
        QCOMPARE(state.memoryLimit(), LuaState::s_defaultMemoryLimit);
        const std::size_t initial = state.memoryUsed();
        QVERIFY(initial > 0);

        QCOMPARE(state.run("big = string.rep('x', 1024 * 1024)", "grow"), std::nullopt);
        QVERIFY(state.memoryUsed() >= initial + 1024 * 1024);

        QCOMPARE(state.run("big = nil; collectgarbage()", "shrink"), std::nullopt);
        QVERIFY(state.memoryUsed() < initial + 1024 * 1024);
    }

    void test_memoryLimitStopsAllocations()
    {
        const std::size_t limit = 1024 * 1024;
        LuaState state(limit);
        QVERIFY(state.isValid());

        auto error = state.run("local t = {} for i = 1, 1e8 do t[i] = i end", "table");
        QVERIFY(error);
        QVERIFY2(error->contains("not enough memory"), qPrintable(*error));
        QVERIFY(state.memoryUsed() <= limit);

        error = state.run("local s = string.rep('x', 2 * 1024 * 1024)", "string");
        QVERIFY(error);
        QVERIFY2(error->contains("not enough memory"), qPrintable(*error));

        // The state is still usable after running out of memory
        QCOMPARE(state.run("collectgarbage(); ok = 'yes'", "after"), std::nullopt);
        QCOMPARE(globalString(state, "ok"), "yes");
    }

    void test_memoryErrorCanBeCaught()
    {
        LuaState state(1024 * 1024);
        QCOMPARE(state.run(R"(
            local ok, err = pcall(string.rep, 'x', 2 * 1024 * 1024)
            assert(not ok and err:find('not enough memory'))
        )",
                           "pcall"),
                 std::nullopt);
    }

    void test_tooSmallLimit()
    {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("^Failed to (create Lua state|open Lua libraries)"));
        LuaState state(1024);
        QVERIFY(!state.isValid());
        QCOMPARE(state.run("x = 1", "invalid"), QString("Lua state is not initialized"));
    }
};

QTEST_GUILESS_MAIN(LuaStateTest)

#include "LuaState_test.moc"
