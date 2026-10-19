#include <QDir>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <plugins/Plugin.h>
#include <plugins/PluginManager.h>
#include <settings/INISettingsObject.h>
#include <settings/Setting.h>

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
