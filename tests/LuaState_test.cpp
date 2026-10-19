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

    void test_infiniteLoopIsStopped()
    {
        LuaState state;
        QCOMPARE(state.instructionLimit(), LuaState::s_defaultInstructionLimit);

        auto error = state.run("while true do end", "loop");
        QVERIFY(error);
        QVERIFY2(error->contains("instruction limit exceeded"), qPrintable(*error));

        // The state is still usable, with a fresh budget
        QCOMPARE(state.run("ok = 'yes'", "after"), std::nullopt);
        QCOMPARE(globalString(state, "ok"), "yes");
    }

    void test_pcallCannotEscapeLimit()
    {
        LuaState state;
        auto error = state.run(R"(
            while true do
                pcall(function() while true do end end)
            end
        )",
                               "pcall");
        QVERIFY(error);
        QVERIFY2(error->contains("instruction limit exceeded"), qPrintable(*error));
    }

    void test_coroutinesShareLimit()
    {
        LuaState state;
        auto error = state.run("coroutine.wrap(function() while true do end end)()", "wrap");
        QVERIFY(error);
        QVERIFY2(error->contains("instruction limit exceeded"), qPrintable(*error));

        // resume catches errors like pcall does
        error = state.run(R"(
            while true do
                coroutine.resume(coroutine.create(function() while true do end end))
            end
        )",
                          "resume");
        QVERIFY(error);
        QVERIFY2(error->contains("instruction limit exceeded"), qPrintable(*error));
    }

    void test_limitIsPerCall()
    {
        LuaState state;
        state.setInstructionLimit(100'000);
        // Each run stays under the limit, all of them together do not
        for (int i = 0; i < 20; i++) {
            QCOMPARE(state.run("for i = 1, 10000 do end", "loop"), std::nullopt);
        }

        auto error = state.run("for i = 1, 1000000 do end", "big");
        QVERIFY(error);
        QVERIFY2(error->contains("instruction limit exceeded"), qPrintable(*error));
    }

    void test_finalizersAreRejected()
    {
        LuaState state;
        auto error = state.run("setmetatable({}, { __gc = function() while true do end end })", "gc");
        QVERIFY(error);
        QVERIFY2(error->contains("__gc metamethods are not allowed"), qPrintable(*error));

        // Adding __gc after setmetatable does not mark the object, so the finalizer never runs
        QCOMPARE(state.run(R"(
            local mt = {}
            setmetatable({}, mt)
            mt.__gc = function() while true do end end
            collectgarbage()
            assert(getmetatable(setmetatable({}, { __index = {} })) ~= nil)
        )",
                           "late gc"),
                 std::nullopt);
        // Closing the state must not hang either
    }

    void test_call()
    {
        LuaState state;
        QCOMPARE(state.run("function add(a, b) result = a + b return result end", "def"), std::nullopt);

        lua_State* L = state.get();
        lua_getglobal(L, "add");
        lua_pushinteger(L, 2);
        lua_pushinteger(L, 3);
        QCOMPARE(state.call(2), std::nullopt);
        QCOMPARE(lua_gettop(L), 0);
        QCOMPARE(globalString(state, "result"), "5");

        lua_getglobal(L, "add");
        lua_pushinteger(L, 2);
        lua_pushboolean(L, true);
        auto error = state.call(2);
        QVERIFY(error);
        QVERIFY2(error->contains("attempt to perform arithmetic") && error->contains("stack traceback:"), qPrintable(*error));
        QCOMPARE(lua_gettop(L), 0);

        // Each call gets a fresh budget
        state.setInstructionLimit(100'000);
        QCOMPARE(state.run("function spin(n) for i = 1, n do end end", "def"), std::nullopt);
        for (int i = 0; i < 10; i++) {
            lua_getglobal(L, "spin");
            lua_pushinteger(L, 20'000);
            QCOMPARE(state.call(1), std::nullopt);
        }
    }

    void test_nestedCallsShareBudget()
    {
        LuaState state;
        state.setInstructionLimit(100'000);
        QCOMPARE(state.run("function small() for i = 1, 5000 do end end", "def"), std::nullopt);

        // A C function that calls back into Lua, as a launcher signal handled during a call would
        lua_pushlightuserdata(state.get(), &state);
        lua_pushcclosure(
            state.get(),
            [](lua_State* L) -> int {
                auto* self = static_cast<LuaState*>(lua_touserdata(L, lua_upvalueindex(1)));
                lua_getglobal(L, "small");
                bool failed = false;
                {
                    auto error = self->call(0);
                    failed = error.has_value();
                }
                if (failed) {
                    return luaL_error(L, "nested call failed");
                }
                return 0;
            },
            1);
        lua_setglobal(state.get(), "nested");

        // Each nested call fits the limit, all of them together do not
        auto error = state.run("for i = 1, 100 do nested() end", "outer");
        QVERIFY(error);
        QVERIFY2(error->contains("nested call failed") || error->contains("instruction limit exceeded"), qPrintable(*error));

        // A new top-level call starts over
        QCOMPARE(state.run("nested()", "again"), std::nullopt);
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
