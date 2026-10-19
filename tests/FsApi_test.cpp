#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <plugins/PluginManifest.h>
#include <plugins/lua/FsApi.h>
#include <plugins/lua/LuaState.h>
#include <plugins/lua/PrismApi.h>

#include <memory>

namespace {

struct FakeSource : PrismApi::InstanceSource {
    QList<PrismApi::InstanceInfo> instances() const override { return { { "game", "Game", "", false, 0, 0 } }; }
    QString gameRoot(const QByteArray& id) const override { return id == "game" ? root : QString(); }
    QString root;
};

}  // namespace

class FsApiTest : public QObject {
    Q_OBJECT

    std::unique_ptr<QTemporaryDir> m_dir;
    FakeSource m_source;

    QString pluginDir() const { return m_dir->filePath("plugin"); }
    QString gameDir() const { return m_dir->filePath("game"); }
    QString outsideDir() const { return m_dir->filePath("outside"); }

    std::unique_ptr<LuaState> newState(const QStringList& permissions = { "fs.instance" })
    {
        PluginManifest manifest;
        manifest.id = "com.example.fs";
        manifest.permissions = permissions;
        auto state = std::make_unique<LuaState>();
        if (auto error = PrismApi::install(*state, manifest, pluginDir(), &m_source)) {
            qFatal("install failed: %s", qPrintable(*error));
        }
        return state;
    }

    static void runOk(LuaState& state, const QByteArray& code)
    {
        auto error = state.run(code, "test");
        QVERIFY2(!error, qPrintable(error.value_or("")));
    }

    static void writeFile(const QString& path, const QByteArray& contents)
    {
        QVERIFY(QDir().mkpath(QFileInfo(path).path()));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(contents);
    }

   private slots:
    void init()
    {
        m_dir = std::make_unique<QTemporaryDir>();
        QVERIFY(m_dir->isValid());
        QVERIFY(QDir().mkpath(pluginDir()));
        writeFile(gameDir() + "/saves/World/level.dat", "level");
        writeFile(gameDir() + "/options.txt", "fov:70\n");
        writeFile(outsideDir() + "/secret.txt", "secret");
        m_source.root = gameDir();
    }

    void cleanup() { m_dir.reset(); }

    void test_dataFolder()
    {
        auto state = newState({});
        runOk(*state, R"(
            local data = prism.fs.data()
            assert(not data:exists("notes.txt"))
            data:write("notes.txt", "hello")
            data:append("notes.txt", " world")
            assert(data:read("notes.txt") == "hello world")
            data:write("sub/deep/file.bin", "\0\1\2")
            assert(data:read("sub/deep/file.bin") == "\0\1\2")
            assert(data:isDir("sub") and not data:isDir("notes.txt"))
            data:mkdir("empty")

            local names = {}
            for _, entry in ipairs(data:list()) do
                table.insert(names, entry.name .. (entry.dir and "/" or ":" .. entry.size))
            end
            assert(table.concat(names, ",") == "empty/,sub/,notes.txt:11", table.concat(names, ","))

            data:remove("notes.txt")
            data:remove("empty")
            local ok, err = pcall(data.remove, data, "sub")
            assert(not ok and err:find("recursive = true", 1, true), err)
            data:remove("sub", { recursive = true })
            assert(#data:list() == 0)
        )");
        // It is the plugin's data folder
        QVERIFY(QDir(pluginDir() + "/data").exists());
    }

    void test_pathsCannotEscape_data()
    {
        QTest::addColumn<QString>("path");
        QTest::newRow("parent") << "../plugin.toml";
        QTest::newRow("parent inside") << "a/../../plugin.toml";
        QTest::newRow("absolute") << "/etc/passwd";
        QTest::newRow("drive") << "C:/Windows/win.ini";
        QTest::newRow("backslash") << "..\\\\plugin.toml";
    }

    void test_pathsCannotEscape()
    {
        QFETCH(QString, path);
        auto state = newState();
        for (const char* op : { "read", "exists", "list", "remove" }) {
            auto error = state->run(QString("prism.fs.data():%1('%2')").arg(op, path).toUtf8(), "test");
            QVERIFY2(error, op);
            QVERIFY2(error->contains("outside of the folder") || error->contains("must be a relative path"), qPrintable(*error));
        }
        auto error = state->run(QString("prism.fs.data():write('%1', 'x')").arg(path).toUtf8(), "test");
        QVERIFY(error);
    }

    void test_symlinksCannotEscape()
    {
        QVERIFY(QDir().mkpath(pluginDir() + "/data"));
        QVERIFY(QFile::link(outsideDir() + "/secret.txt", pluginDir() + "/data/link.txt"));
        QVERIFY(QFile::link(outsideDir(), pluginDir() + "/data/linkdir"));

        auto state = newState();
        for (const char* code :
             { "prism.fs.data():read('link.txt')", "prism.fs.data():write('link.txt', 'x')", "prism.fs.data():read('linkdir/secret.txt')",
               "prism.fs.data():write('linkdir/new.txt', 'x')", "prism.fs.data():list('linkdir')" }) {
            auto error = state->run(code, "test");
            QVERIFY2(error && error->contains("outside of the folder"), code);
        }
        QVERIFY(!QFile::exists(outsideDir() + "/new.txt"));
        QCOMPARE(QFile(outsideDir() + "/secret.txt").size(), 6);

        // The link itself can be removed, without touching its target
        runOk(*state, "prism.fs.data():remove('link.txt')");
        QVERIFY(QFile::exists(outsideDir() + "/secret.txt"));

        // Removing a folder recursively does not follow the links inside it
        QVERIFY(QDir().mkpath(pluginDir() + "/data/folder"));
        QVERIFY(QFile::link(outsideDir(), pluginDir() + "/data/folder/inner"));
        runOk(*state, "prism.fs.data():remove('folder', { recursive = true })");
        QVERIFY(!QFileInfo::exists(pluginDir() + "/data/folder"));
        QVERIFY(QFile::exists(outsideDir() + "/secret.txt"));
    }

    void test_rootIsProtected()
    {
        auto state = newState();
        runOk(*state, "prism.fs.data():write('x', 'y')");
        for (const char* code :
             { "prism.fs.data():remove('.', { recursive = true })", "prism.fs.data():remove('a/..')", "prism.fs.data():write('.', 'x')" }) {
            QVERIFY2(state->run(code, "test"), code);
        }
        QVERIFY(QFile::exists(pluginDir() + "/data/x"));
    }

    void test_instanceFolder()
    {
        auto state = newState();
        runOk(*state, R"lua(
            local game = prism.fs.instance("game")
            assert(game:read("options.txt") == "fov:70\n")
            assert(game:isDir("saves/World"))
            game:write("options.txt", "fov:90\n")

            -- Back up a world into the plugin's data, then restore it under another name
            local data = prism.fs.data()
            prism.fs.copy(game, "saves/World", data, "backups/World")
            assert(data:read("backups/World/level.dat") == "level")
            prism.fs.move(data, "backups/World", game, "saves/World (restored)")
            assert(game:read("saves/World (restored)/level.dat") == "level")
            assert(not data:exists("backups/World"))

            local function fails(fn, expected)
                local ok, err = pcall(fn)
                assert(not ok and err:find(expected, 1, true), tostring(err))
            end
            fails(function() prism.fs.copy(game, "saves/World", game, "saves/World") end, "already exists")
            fails(function() prism.fs.copy(game, "saves", game, "saves/inner") end, "into itself")
            fails(function() prism.fs.copy(game, "missing", data, "x") end, "does not exist")
            fails(function() prism.fs.instance("nope") end, "the instance 'nope' does not exist")
        )lua");
        QFile options(gameDir() + "/options.txt");
        QVERIFY(options.open(QIODevice::ReadOnly));
        QCOMPARE(options.readAll(), "fov:90\n");
    }

    void test_instanceNeedsPermission()
    {
        auto state = newState({});
        auto error = state->run("prism.fs.instance('game')", "test");
        QVERIFY(error && error->contains("prism.fs.instance requires the 'fs.instance' permission"));
    }

    void test_readLimit()
    {
        writeFile(pluginDir() + "/data/big.bin", QByteArray(PrismApi::s_maxFileReadSize + 1, 'x'));
        auto state = newState();
        auto error = state->run("prism.fs.data():read('big.bin')", "test");
        QVERIFY(error && error->contains("is larger than"));
    }

    void test_handlesAreOpaque()
    {
        auto state = newState();
        runOk(*state, R"(
            local data = prism.fs.data()
            assert(getmetatable(data) == false)
            assert(not pcall(prism.fs.copy, {}, "a", data, "b"))
            assert(not pcall(data.read, {}, "a"))
        )");
    }
};

QTEST_GUILESS_MAIN(FsApiTest)

#include "FsApi_test.moc"
