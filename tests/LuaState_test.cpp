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
};

QTEST_GUILESS_MAIN(LuaStateTest)

#include "LuaState_test.moc"
