#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>

#include <functional>
#include <memory>

#include <FileSystem.h>
#include <plugins/Plugin.h>
#include <plugins/lua/LuaState.h>
#include <plugins/lua/PrismApi.h>

// Runs the example plugins of examples/plugins against a fake launcher
namespace {

struct FakeHost : PrismApi::InstanceSource, PrismApi::HttpClient, PrismApi::Notifier {
    QList<PrismApi::InstanceInfo> instances() const override { return { { "game", "Game", "", false, 0, 3600 } }; }
    QString gameRoot(const QByteArray& id) const override { return id == "game" ? root : QString(); }
    void send(const PrismApi::HttpRequest& request, std::function<void(const PrismApi::HttpResponse&)> done) override
    {
        requests.append(request);
        pending.append(std::move(done));
    }
    void notify(const QString&, const QString& text, PrismApi::NotifyLevel) override { notifications.append(text); }

    QString root;
    QList<PrismApi::HttpRequest> requests;
    QList<std::function<void(const PrismApi::HttpResponse&)>> pending;
    QStringList notifications;
};

}  // namespace

class ExamplePluginsTest : public QObject {
    Q_OBJECT

    std::unique_ptr<QTemporaryDir> m_dir;
    FakeHost m_host;

    QString pluginsDir() const { return m_dir->filePath("plugins"); }

    std::unique_ptr<Plugin> loadExample(const QString& id)
    {
        if (!FS::copy(QString(EXAMPLES_DIR) + "/" + id, QDir(pluginsDir()).filePath(id))()) {
            qFatal("cannot copy %s", qPrintable(id));
        }
        auto plugin = std::make_unique<Plugin>(QDir(pluginsDir()).filePath(id), PrismApi::Host{ &m_host, &m_host, &m_host });
        if (!plugin->load()) {
            qFatal("%s did not load: %s", qPrintable(id), qPrintable(plugin->error()));
        }
        return plugin;
    }

    static int actionIndex(Plugin& plugin, const QString& text)
    {
        QList<PrismApi::UiActionInfo> actions;
        PrismApi::uiActions(*plugin.lua(), actions);
        for (const auto& action : actions) {
            if (action.text == text) {
                return action.index;
            }
        }
        return -1;
    }

    static QDialog* findWindow(const QString& title)
    {
        for (QWidget* widget : QApplication::topLevelWidgets()) {
            if (auto* dialog = qobject_cast<QDialog*>(widget); dialog && dialog->windowTitle() == title && dialog->isVisible()) {
                return dialog;
            }
        }
        return nullptr;
    }

    static QPushButton* button(QDialog* window, const QString& text)
    {
        for (auto* button : window->findChildren<QPushButton*>()) {
            if (button->text() == text) {
                return button;
            }
        }
        return nullptr;
    }

   private slots:
    void init()
    {
        m_dir = std::make_unique<QTemporaryDir>();
        QVERIFY(m_dir->isValid());
        m_host = {};
        m_host.root = m_dir->filePath("game");
        QVERIFY(QDir().mkpath(m_host.root + "/saves/World"));
        QFile level(m_host.root + "/saves/World/level.dat");
        QVERIFY(level.open(QIODevice::WriteOnly));
        level.write("level");
    }

    void cleanup() { m_dir.reset(); }

    void test_worldBackup()
    {
        auto plugin = loadExample("prism.worldbackup");
        QList<PrismApi::LaunchStepInfo> steps;
        QCOMPARE(PrismApi::launchSteps(*plugin->lua(), steps), std::nullopt);
        QCOMPARE(steps.size(), 1);
        QCOMPARE(steps.first().anchor, "launch");

        // Four launches keep the last three backups
        QStringList logged;
        PrismApi::LaunchContext context;
        context.instanceId = "game";
        context.instanceName = "Game";
        context.log = [&logged](const QString& line) { logged.append(line); };
        for (int i = 0; i < 4; i++) {
            QCOMPARE(PrismApi::runLaunchStep(*plugin->lua(), steps.first().index, context), std::nullopt);
        }
        QCOMPARE(logged.last(), "Worlds backed up (backup 4)");
        const QString backups = plugin->directory() + "/data/game";
        QCOMPARE(QDir(backups).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name), QStringList({ "2", "3", "4" }));
        QVERIFY(QFile::exists(backups + "/4/World/level.dat"));

        // The window restores a world next to the current ones, never over them
        const PrismApi::InstanceInfo instance{ "game", "Game", "", false, 0, 0 };
        QCOMPARE(PrismApi::runUiAction(*plugin->lua(), actionIndex(*plugin, "World backups"), &instance), std::nullopt);
        QDialog* window = findWindow("World backups - Game");
        QVERIFY(window);
        auto* list = window->findChild<QListWidget*>();
        QCOMPARE(list->count(), 3);
        QCOMPARE(list->item(0)->text(), "Backup 4: World");
        list->setCurrentRow(0);
        QTest::mouseClick(button(window, "Restore"), Qt::LeftButton);
        QVERIFY(QFile::exists(m_host.root + "/saves/World (backup 4)/level.dat"));
        QTest::mouseClick(button(window, "Restore"), Qt::LeftButton);
        QVERIFY(window->findChild<QLabel*>()->text().contains("already exists"));
        window->close();
    }

    void test_playTime()
    {
        auto plugin = loadExample("prism.playtime");
        for (int i = 0; i < 2; i++) {
            QCOMPARE(PrismApi::emitEvent(*plugin->lua(), "launch.finished",
                                         { { "id", QByteArray("game") }, { "success", true }, { "reason", QByteArray() } }),
                     std::nullopt);
        }
        QVERIFY(QFile::exists(plugin->directory() + "/data/sessions.json"));

        QCOMPARE(PrismApi::runUiAction(*plugin->lua(), actionIndex(*plugin, "Play time"), nullptr), std::nullopt);
        QDialog* window = findWindow("Play time");
        QVERIFY(window);
        QCOMPARE(window->findChild<QListWidget*>()->item(0)->text(), "Game - 1h 00m - 2 sessions");

        // The count survives a restart of the plugin
        window->close();
        plugin->unload();
        QVERIFY(plugin->load());
        QCOMPARE(PrismApi::runUiAction(*plugin->lua(), actionIndex(*plugin, "Play time"), nullptr), std::nullopt);
        QCOMPARE(findWindow("Play time")->findChild<QListWidget*>()->item(0)->text(), "Game - 1h 00m - 2 sessions");
        findWindow("Play time")->close();
    }

    void test_minecraftReleases()
    {
        auto plugin = loadExample("prism.mcnews");
        auto answer = [this](const QByteArray& release, const QByteArray& snapshot) {
            m_host.pending.takeFirst()(
                { 200, "{\"latest\":{\"release\":\"" + release + "\",\"snapshot\":\"" + snapshot + "\"},\"versions\":[]}", {}, {} });
        };

        // The first check only remembers the versions
        QCOMPARE(PrismApi::emitEvent(*plugin->lua(), "launcher.ready"), std::nullopt);
        QCOMPARE(m_host.requests.last().url.host(), "piston-meta.mojang.com");
        answer("1.21", "25w01a");
        QVERIFY(m_host.notifications.isEmpty());

        // Later ones tell what is new
        QCOMPARE(PrismApi::emitEvent(*plugin->lua(), "launcher.ready"), std::nullopt);
        answer("1.22", "25w05a");
        QCOMPARE(m_host.notifications, QStringList({ "Minecraft 1.22 is out!", "New snapshot: 25w05a" }));

        // Asking by hand always answers
        m_host.notifications.clear();
        QCOMPARE(PrismApi::runUiAction(*plugin->lua(), actionIndex(*plugin, "Check Minecraft versions"), nullptr), std::nullopt);
        answer("1.22", "25w05a");
        QCOMPARE(m_host.notifications, QStringList({ "Latest Minecraft: 1.22 (snapshot 25w05a)" }));

        // Failures are logged, and reported when asked by hand
        m_host.notifications.clear();
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Could not check for new Minecraft versions"));
        QCOMPARE(PrismApi::runUiAction(*plugin->lua(), actionIndex(*plugin, "Check Minecraft versions"), nullptr), std::nullopt);
        m_host.pending.takeFirst()({ 0, {}, {}, "Host not found" });
        QCOMPARE(m_host.notifications, QStringList({ "Could not reach Mojang to check for new versions." }));
    }
};

int main(int argc, char* argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    ExamplePluginsTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "ExamplePlugins_test.moc"
