#include <QDir>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QString>
#include <QTemporaryDir>
#include <QTest>

#include <archive/ArchiveWriter.h>
#include <plugins/Plugin.h>
#include <plugins/PluginManager.h>
#include <plugins/lua/LuaState.h>
#include <settings/INISettingsObject.h>
#include <settings/Setting.h>

#include <lua.hpp>
#include <memory>

class PluginManagerTest : public QObject {
    Q_OBJECT

    std::unique_ptr<QTemporaryDir> m_root;
    std::unique_ptr<INISettingsObject> m_settings;
    std::shared_ptr<Setting> m_enabled;

    QString pluginsDir() const { return m_root->filePath("plugins"); }

    void addPlugin(const QString& id, const QByteArray& code = "loaded = true")
    {
        QDir dir(pluginsDir());
        QVERIFY(dir.mkpath(id));
        writeFile(dir.filePath(id + "/plugin.toml"), "id = \"" + id.toUtf8() + "\"\nname = \"Test\"\nversion = \"1\"\napi = 1\n");
        writeFile(dir.filePath(id + "/main.lua"), code);
    }

    static void writeFile(const QString& path, const QByteArray& contents)
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(contents);
    }

    static QStringList ids(const PluginManager& manager)
    {
        QStringList ids;
        for (Plugin* plugin : manager.plugins()) {
            ids.append(plugin->id());
        }
        return ids;
    }

   private slots:
    void init()
    {
        m_root = std::make_unique<QTemporaryDir>();
        QVERIFY(m_root->isValid());
        m_settings = std::make_unique<INISettingsObject>(m_root->filePath("settings.cfg"));
        m_enabled = m_settings->registerSetting("EnabledPlugins", QStringList());
    }

    void cleanup()
    {
        m_enabled.reset();
        m_settings.reset();
        m_root.reset();
    }

    void test_createsMissingFolder()
    {
        PluginManager manager(pluginsDir(), m_enabled);
        QSignalSpy changed(&manager, &PluginManager::pluginsChanged);
        manager.discover();
        QVERIFY(QDir(pluginsDir()).exists());
        QVERIFY(manager.plugins().isEmpty());
        QCOMPARE(changed.count(), 1);
    }

    void test_discoversFoldersOnly()
    {
        addPlugin("com.example.b");
        addPlugin("com.example.a");
        QVERIFY(QDir(pluginsDir()).mkpath(".hidden"));
        QVERIFY(QDir(pluginsDir()).mkpath("broken"));
        writeFile(QDir(pluginsDir()).filePath("README.txt"), "not a plugin");

        PluginManager manager(pluginsDir(), m_enabled);
        manager.discover();
        QCOMPARE(ids(manager), QStringList({ "broken", "com.example.a", "com.example.b" }));
        QCOMPARE(manager.plugin("broken")->state(), Plugin::State::Error);
        QVERIFY(!manager.plugin("missing"));

        // New plugins start disabled
        QCOMPARE(manager.plugin("com.example.a")->state(), Plugin::State::Disabled);
        QVERIFY(!manager.isEnabled("com.example.a"));
    }

    void test_loadsEnabledPlugins()
    {
        addPlugin("com.example.a");
        addPlugin("com.example.b");
        m_enabled->set(QStringList({ "com.example.b" }));

        PluginManager manager(pluginsDir(), m_enabled);
        manager.discover();
        QCOMPARE(manager.plugin("com.example.a")->state(), Plugin::State::Disabled);
        QCOMPARE(manager.plugin("com.example.b")->state(), Plugin::State::Loaded);
    }

    void test_enableAndDisable()
    {
        addPlugin("com.example.a");
        addPlugin("com.example.b");
        PluginManager manager(pluginsDir(), m_enabled);
        manager.discover();
        QSignalSpy changed(&manager, &PluginManager::pluginChanged);

        QVERIFY(manager.setEnabled("com.example.b", true));
        QVERIFY(manager.setEnabled("com.example.a", true));
        QCOMPARE(manager.plugin("com.example.a")->state(), Plugin::State::Loaded);
        QCOMPARE(m_enabled->get().toStringList(), QStringList({ "com.example.a", "com.example.b" }));
        QCOMPARE(changed.count(), 2);
        QCOMPARE(changed.at(0).at(0).toString(), "com.example.b");

        QVERIFY(manager.setEnabled("com.example.a", false));
        QCOMPARE(manager.plugin("com.example.a")->state(), Plugin::State::Disabled);
        QVERIFY(!manager.isEnabled("com.example.a"));
        QCOMPARE(m_enabled->get().toStringList(), QStringList({ "com.example.b" }));
        QCOMPARE(changed.count(), 3);

        // The choice is saved to disk
        INISettingsObject reloaded(m_root->filePath("settings.cfg"));
        QCOMPARE(reloaded.registerSetting("EnabledPlugins", QStringList())->get().toStringList(), QStringList({ "com.example.b" }));
    }

    void test_unknownPlugin()
    {
        PluginManager manager(pluginsDir(), m_enabled);
        manager.discover();
        QSignalSpy changed(&manager, &PluginManager::pluginChanged);
        QVERIFY(!manager.setEnabled("com.example.missing", true));
        QVERIFY(!manager.isEnabled("com.example.missing"));
        QCOMPARE(changed.count(), 0);
    }

    void test_enableFailingPlugin()
    {
        addPlugin("com.example.bad", "error('boom')");
        PluginManager manager(pluginsDir(), m_enabled);
        manager.discover();

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("^Failed to load plugin \"com.example.bad\""));
        QVERIFY(!manager.setEnabled("com.example.bad", true));
        QCOMPARE(manager.plugin("com.example.bad")->state(), Plugin::State::Error);
        // Stays enabled, so it is retried on the next start
        QVERIFY(manager.isEnabled("com.example.bad"));

        QVERIFY(manager.setEnabled("com.example.bad", false));
        QCOMPARE(manager.plugin("com.example.bad")->state(), Plugin::State::Disabled);
    }

    void test_keepsChoicesOfMissingPlugins()
    {
        addPlugin("com.example.a");
        m_enabled->set(QStringList({ "com.example.gone" }));
        PluginManager manager(pluginsDir(), m_enabled);
        manager.discover();

        QVERIFY(manager.setEnabled("com.example.a", true));
        QCOMPARE(m_enabled->get().toStringList(), QStringList({ "com.example.a", "com.example.gone" }));
    }

    void test_emitEvent()
    {
        addPlugin("com.example.a", "prism.on('launcher.ready', function() prism.log.info('ready') end)");
        addPlugin("com.example.b", "prism.on('launcher.ready', function() error('boom') end)");
        addPlugin("com.example.off", "prism.on('launcher.ready', function() prism.log.info('should not run') end)");
        m_enabled->set(QStringList({ "com.example.a", "com.example.b" }));
        PluginManager manager(pluginsDir(), m_enabled);
        manager.discover();

        QTest::ignoreMessage(QtInfoMsg, "[com.example.a] ready");
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("^\\[com.example.b\\] error handling launcher.ready: main.lua:1: boom"));
        manager.emitEvent("launcher.ready");
        // A failing handler does not unload the plugin
        QCOMPARE(manager.plugin("com.example.b")->state(), Plugin::State::Loaded);
    }

    void test_lateLoadsGetLauncherReady()
    {
        addPlugin("com.example.early", "prism.on('launcher.ready', function() prism.log.info('early ready') end)");
        addPlugin("com.example.late", "prism.on('launcher.ready', function() prism.log.info('late ready') end)");
        m_enabled->set(QStringList({ "com.example.early" }));
        PluginManager manager(pluginsDir(), m_enabled);
        manager.discover();

        QTest::ignoreMessage(QtInfoMsg, "[com.example.early] early ready");
        manager.setLauncherReady();

        // Enabled after the launcher was ready: gets the event right after loading
        QTest::ignoreMessage(QtInfoMsg, "[com.example.late] late ready");
        QVERIFY(manager.setEnabled("com.example.late", true));

        // Reloading restarts the plugins, which get it again
        QTest::ignoreMessage(QtInfoMsg, "[com.example.early] early ready");
        QTest::ignoreMessage(QtInfoMsg, "[com.example.late] late ready");
        manager.discover();
    }

    void test_actions()
    {
        addPlugin("com.example.ui", R"(
            prism.ui.addAction{ text = "Hello", scope = "launcher", run = function() prism.log.info("hello action") end }
            prism.ui.addAction{ text = "Per instance", scope = "instance", run = function(ctx) end }
            prism.ui.addAction{ text = "Fails", scope = "launcher", run = function() error("broken action") end }
        )");
        addPlugin("com.example.plain");
        QDir dir(pluginsDir());
        writeFile(dir.filePath("com.example.ui/plugin.toml"),
                  "id = \"com.example.ui\"\nname = \"UI plugin\"\nversion = \"1\"\napi = 1\npermissions = [\"ui\"]\n");

        PluginManager manager(pluginsDir(), m_enabled);
        manager.discover();
        QVERIFY(!manager.hasUiPlugins());

        m_enabled->set(QStringList({ "com.example.ui", "com.example.plain" }));
        manager.discover();
        QVERIFY(manager.hasUiPlugins());

        const auto launcherActions = manager.actions(PrismApi::ActionScope::Launcher);
        QCOMPARE(launcherActions.size(), 2);
        QCOMPARE(launcherActions[0].pluginName, "UI plugin");
        QCOMPARE(launcherActions[0].text, "Hello");
        QCOMPARE(manager.actions(PrismApi::ActionScope::Instance).size(), 1);

        QTest::ignoreMessage(QtInfoMsg, "[com.example.ui] hello action");
        QCOMPARE(manager.runAction(launcherActions[0]), std::nullopt);

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("action \"Fails\" failed: .*broken action"));
        auto error = manager.runAction(launcherActions[1]);
        QVERIFY(error && error->contains("broken action"));

        // Without the instance list there is no such instance
        error = manager.runAction(manager.actions(PrismApi::ActionScope::Instance).first(), "missing");
        QVERIFY(error && error->contains("does not exist"));

        manager.setEnabled("com.example.ui", false);
        QVERIFY(!manager.hasUiPlugins());
        error = manager.runAction(launcherActions[0]);
        QVERIFY(error && error->contains("is not running"));
    }

    void test_notificationRateLimit()
    {
        addPlugin("com.example.noisy", "for i = 1, 7 do prism.ui.notify('message ' .. i) end");
        writeFile(QDir(pluginsDir()).filePath("com.example.noisy/plugin.toml"),
                  "id = \"com.example.noisy\"\nname = \"Noisy\"\nversion = \"1\"\napi = 1\npermissions = [\"ui\"]\n");
        m_enabled->set(QStringList({ "com.example.noisy" }));
        PluginManager manager(pluginsDir(), m_enabled);
        QSignalSpy notifications(&manager, &PluginManager::notificationRequested);

        for (int i = 1; i <= PluginManager::s_maxNotificationsPerMinute; i++) {
            QTest::ignoreMessage(QtInfoMsg, qPrintable(QString("[com.example.noisy] notification: message %1").arg(i)));
        }
        QTest::ignoreMessage(QtWarningMsg, "[com.example.noisy] notification dropped, too many in the last minute: message 6");
        QTest::ignoreMessage(QtWarningMsg, "[com.example.noisy] notification dropped, too many in the last minute: message 7");
        manager.discover();

        QCOMPARE(notifications.size(), PluginManager::s_maxNotificationsPerMinute);
        QCOMPARE(notifications.first().at(1).toString(), "Noisy");
        QCOMPARE(notifications.first().at(2).toString(), "message 1");
    }

    void test_permissionsMustBeGranted()
    {
        addPlugin("com.example.perm");
        const QString toml = QDir(pluginsDir()).filePath("com.example.perm/plugin.toml");
        writeFile(toml, "id = \"com.example.perm\"\nname = \"Perm\"\nversion = \"1\"\napi = 1\npermissions = [\"net\"]\n");
        auto grants = m_settings->registerSetting("PluginPermissions", QString("{}"));

        PluginManager manager(pluginsDir(), m_enabled);
        manager.setGrantsSetting(grants);
        manager.discover();
        QCOMPARE(manager.missingPermissions("com.example.perm"), QStringList({ "net" }));

        // Enabling without granting does not load it
        QTest::ignoreMessage(QtWarningMsg,
                             QRegularExpression("^Not loading plugin \"com.example.perm\" until its permissions are granted"));
        QVERIFY(!manager.setEnabled("com.example.perm", true));
        QCOMPARE(manager.plugin("com.example.perm")->state(), Plugin::State::Error);
        QVERIFY(manager.plugin("com.example.perm")->error().contains("Requests permissions you have not granted: net"));

        manager.grantPermissions("com.example.perm");
        QVERIFY(manager.missingPermissions("com.example.perm").isEmpty());
        QVERIFY(manager.setEnabled("com.example.perm", true));
        QCOMPARE(manager.plugin("com.example.perm")->state(), Plugin::State::Loaded);

        // An update asking for more is blocked until granted again
        writeFile(toml, "id = \"com.example.perm\"\nname = \"Perm\"\nversion = \"2\"\napi = 1\npermissions = [\"net\", \"ui\"]\n");
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("^Not loading plugin \"com.example.perm\""));
        manager.discover();
        QCOMPARE(manager.missingPermissions("com.example.perm"), QStringList({ "ui" }));
        QCOMPARE(manager.plugin("com.example.perm")->state(), Plugin::State::Error);

        // The grants are kept outside the plugin, in the launcher settings
        QVERIFY(grants->get().toString().contains("com.example.perm"));
    }

    static QString globalString(Plugin* plugin, const char* name)
    {
        if (!plugin->lua()) {
            return {};
        }
        lua_State* L = plugin->lua()->get();
        lua_getglobal(L, name);
        QString value = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        return value;
    }

    void test_autoReload()
    {
        addPlugin("com.example.live", "value = 'first'");
        m_enabled->set(QStringList({ "com.example.live" }));
        PluginManager manager(pluginsDir(), m_enabled);
        manager.setAutoReload(true);
        manager.discover();
        Plugin* plugin = manager.plugin("com.example.live");
        QCOMPARE(globalString(plugin, "value"), "first");
        const int loads = plugin->loadCount();

        // Changing the code reloads it
        QTest::ignoreMessage(QtInfoMsg, QRegularExpression("changed on disk"));
        QTest::ignoreMessage(QtInfoMsg, QRegularExpression("^Loaded plugin"));
        QTest::ignoreMessage(QtInfoMsg, QRegularExpression("^Reloaded plugin"));
        writeFile(QDir(pluginsDir()).filePath("com.example.live/main.lua"), "value = 'second one'");
        QTRY_COMPARE_WITH_TIMEOUT(globalString(plugin, "value"), "second one", 5000);
        QCOMPARE(plugin->loadCount(), loads + 1);

        // A new module in a folder too
        QTest::ignoreMessage(QtInfoMsg, QRegularExpression("changed on disk"));
        QTest::ignoreMessage(QtInfoMsg, QRegularExpression("^Loaded plugin"));
        QTest::ignoreMessage(QtInfoMsg, QRegularExpression("^Reloaded plugin"));
        QVERIFY(QDir(pluginsDir()).mkpath("com.example.live/lib"));
        writeFile(QDir(pluginsDir()).filePath("com.example.live/lib/extra.lua"), "return 1");
        QTRY_COMPARE_WITH_TIMEOUT(plugin->loadCount(), loads + 2, 5000);

        // What the plugin writes itself does not
        QVERIFY(plugin->lua()->run("prism.settings{ n = 0 }.n = 5 prism.fs.data():write(\"notes.txt\", \"x\")", "write") == std::nullopt);
        QTest::qWait(PluginManager::s_reloadDelayMs * 3);
        QCOMPARE(plugin->loadCount(), loads + 2);
    }

    void test_reloadPlugin()
    {
        addPlugin("com.example.manual", "value = 'old'");
        auto grants = m_settings->registerSetting("PluginPermissions", QString("{}"));
        m_enabled->set(QStringList({ "com.example.manual" }));
        PluginManager manager(pluginsDir(), m_enabled);
        manager.setGrantsSetting(grants);
        manager.discover();
        Plugin* plugin = manager.plugin("com.example.manual");

        writeFile(QDir(pluginsDir()).filePath("com.example.manual/main.lua"), "value = 'new'");
        QSignalSpy changed(&manager, &PluginManager::pluginChanged);
        manager.reloadPlugin("com.example.manual");
        QCOMPARE(globalString(plugin, "value"), "new");
        QCOMPARE(changed.count(), 1);

        // The manifest is read again: new permissions block it
        writeFile(QDir(pluginsDir()).filePath("com.example.manual/plugin.toml"),
                  "id = \"com.example.manual\"\nname = \"Renamed\"\nversion = \"2\"\napi = 1\npermissions = [\"net\"]\n");
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("^Not loading plugin"));
        manager.reloadPlugin("com.example.manual");
        QCOMPARE(plugin->manifest()->name, "Renamed");
        QCOMPARE(plugin->state(), Plugin::State::Error);
    }

    // A zip with the files (path -> contents); "->target" contents make a symbolic link instead
    QString makeZip(const QString& name, const QMap<QString, QByteArray>& files)
    {
        const QString path = m_root->filePath(name);
        MMCZip::ArchiveWriter zip(path);
        if (!zip.open()) {
            qFatal("cannot create %s", qPrintable(path));
        }
        for (auto it = files.begin(); it != files.end(); ++it) {
            if (it.value().startsWith("->")) {
                const QString link = m_root->filePath("link-" + QString::number(qHash(it.key())));
                QFile::link(QString::fromUtf8(it.value().mid(2)), link);
                if (!zip.addFile(link, it.key())) {
                    qFatal("cannot add link");
                }
            } else if (!zip.addFile(it.key(), it.value())) {
                qFatal("cannot add %s", qPrintable(it.key()));
            }
        }
        if (!zip.close()) {
            qFatal("cannot close %s", qPrintable(path));
        }
        return path;
    }

    static QByteArray manifestFor(const QByteArray& id, const QByteArray& version = "1")
    {
        return "id = \"" + id + "\"\nname = \"Zipped\"\nversion = \"" + version + "\"\napi = 1\n";
    }

    void test_installFromZip()
    {
        PluginManager manager(pluginsDir(), m_enabled);
        manager.discover();
        const auto never = [](const PluginManifest&, const Plugin&) { return false; };
        const auto always = [](const PluginManifest&, const Plugin&) { return true; };

        // Manifest at the top
        QTest::ignoreMessage(QtInfoMsg, QRegularExpression("^Installed plugin"));
        auto installed = manager.installFromZip(
            makeZip("top.zip",
                    { { "plugin.toml", manifestFor("com.example.zipped") }, { "main.lua", "value = 1" }, { "lib/util.lua", "return 1" } }),
            never);
        QVERIFY2(installed, qPrintable(installed.error_or("")));
        QCOMPARE(*installed, "com.example.zipped");
        QCOMPARE(manager.plugin("com.example.zipped")->state(), Plugin::State::Disabled);
        QVERIFY(QFile::exists(QDir(pluginsDir()).filePath("com.example.zipped/lib/util.lua")));

        // Inside the only folder, as zipping a folder does
        QTest::ignoreMessage(QtInfoMsg, QRegularExpression("^Installed plugin"));
        installed = manager.installFromZip(
            makeZip("folder.zip", { { "whatever/plugin.toml", manifestFor("com.example.folder") }, { "whatever/main.lua", "" } }), never);
        QVERIFY2(installed, qPrintable(installed.error_or("")));
        QVERIFY(manager.plugin("com.example.folder"));

        // Same id: replaced only when allowed, keeping the settings and data
        QVERIFY(manager.setEnabled("com.example.zipped", true));
        writeFile(QDir(pluginsDir()).filePath("com.example.zipped/settings.ini"), "[General]\nConfigVersion=1.3\nn=5\n");
        QVERIFY(QDir(pluginsDir()).mkpath("com.example.zipped/data"));
        writeFile(QDir(pluginsDir()).filePath("com.example.zipped/data/keep.txt"), "kept");
        const QString update =
            makeZip("update.zip", { { "plugin.toml", manifestFor("com.example.zipped", "2") }, { "main.lua", "value = 2" } });
        auto refused = manager.installFromZip(update, never);
        QVERIFY(!refused && refused.error().contains("already installed"));
        QCOMPARE(manager.plugin("com.example.zipped")->manifest()->version, "1");

        QTest::ignoreMessage(QtInfoMsg, QRegularExpression("^Loaded plugin"));
        QTest::ignoreMessage(QtInfoMsg, QRegularExpression("^Reloaded plugin"));
        QTest::ignoreMessage(QtInfoMsg, QRegularExpression("^Installed plugin"));
        installed = manager.installFromZip(update, always);
        QVERIFY2(installed, qPrintable(installed.error_or("")));
        Plugin* updated = manager.plugin("com.example.zipped");
        QCOMPARE(updated->manifest()->version, "2");
        QCOMPARE(updated->state(), Plugin::State::Loaded);
        QVERIFY(QFile::exists(QDir(pluginsDir()).filePath("com.example.zipped/settings.ini")));
        QVERIFY(QFile::exists(QDir(pluginsDir()).filePath("com.example.zipped/data/keep.txt")));
        QVERIFY(!QFile::exists(QDir(pluginsDir()).filePath("com.example.zipped/lib/util.lua")));

        // Nothing is left behind
        QVERIFY(QDir(pluginsDir()).entryList({ ".install-*" }, QDir::Dirs | QDir::Hidden).isEmpty());
    }

    void test_installRejects()
    {
        PluginManager manager(pluginsDir(), m_enabled);
        manager.discover();
        const auto never = [](const PluginManifest&, const Plugin&) { return false; };
        auto expectFailure = [&](const QString& zip, const QString& expected) {
            auto result = manager.installFromZip(zip, never);
            QVERIFY2(!result, qPrintable(zip));
            QVERIFY2(result.error().contains(expected), qPrintable(result.error()));
        };

        expectFailure(makeZip("none.zip", { { "main.lua", "" } }), "no plugin.toml");
        expectFailure(
            makeZip("two.zip", { { "a/plugin.toml", manifestFor("com.example.a") }, { "b/plugin.toml", manifestFor("com.example.b") } }),
            "no plugin.toml");
        expectFailure(makeZip("bad.zip", { { "plugin.toml", "id = " } }), "is not valid");
        expectFailure(makeZip("escape.zip", { { "plugin.toml", manifestFor("com.example.escape") }, { "../outside.lua", "evil" } }),
                      "Could not extract");
        QVERIFY(!QFile::exists(m_root->filePath("outside.lua")));
#ifndef Q_OS_WIN
        // A link out of the plugin is stopped while extracting (installFromZip also rejects any link left afterwards)
        expectFailure(makeZip("link.zip", { { "plugin.toml", manifestFor("com.example.link") }, { "secret", "->/etc/passwd" } }),
                      "outside root");
#endif
        QMap<QString, QByteArray> many = { { "plugin.toml", manifestFor("com.example.many") } };
        for (int i = 0; i < PluginManager::s_maxZipFiles; i++) {
            many.insert(QString("f/%1.txt").arg(i), "x");
        }
        expectFailure(makeZip("many.zip", many), "too large");

        QVERIFY(manager.plugins().isEmpty());
        QVERIFY(QDir(pluginsDir()).entryList({ ".install-*" }, QDir::Dirs | QDir::Hidden).isEmpty());
    }

    void test_rediscover()
    {
        addPlugin("com.example.a");
        m_enabled->set(QStringList({ "com.example.a", "com.example.b" }));
        PluginManager manager(pluginsDir(), m_enabled);
        manager.discover();
        QCOMPARE(manager.plugin("com.example.a")->state(), Plugin::State::Loaded);

        addPlugin("com.example.b");
        manager.discover();
        QCOMPARE(ids(manager), QStringList({ "com.example.a", "com.example.b" }));
        QCOMPARE(manager.plugin("com.example.a")->state(), Plugin::State::Loaded);
        QCOMPARE(manager.plugin("com.example.b")->state(), Plugin::State::Loaded);
    }
};

QTEST_GUILESS_MAIN(PluginManagerTest)

#include "PluginManager_test.moc"
