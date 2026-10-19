#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>

#include <functional>
#include <memory>

#include <plugins/PluginManifest.h>
#include <plugins/lua/LuaState.h>
#include <plugins/lua/PrismApi.h>

// Runs the examples of docs/plugins.md, so they keep working as the API changes
namespace {

struct FakeHost : PrismApi::InstanceSource, PrismApi::HttpClient, PrismApi::Notifier {
    QList<PrismApi::InstanceInfo> instances() const override { return { { "game", "Game", "", false, 0, 0 } }; }
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

class DocExamplesTest : public QObject {
    Q_OBJECT

    QTemporaryDir m_dir;
    FakeHost m_host;

    // The first block of the language after the heading
    static QByteArray example(const QString& heading, const QString& language = "lua")
    {
        QFile file(DOCS_FILE);
        if (!file.open(QIODevice::ReadOnly)) {
            qFatal("cannot open %s", DOCS_FILE);
        }
        const QString text = QString::fromUtf8(file.readAll());
        const qsizetype start = text.indexOf("\n" + heading + "\n");
        if (start < 0) {
            qFatal("no heading %s", qPrintable(heading));
        }
        const QRegularExpression block("```" + language + "\\n(.*?)```", QRegularExpression::DotMatchesEverythingOption);
        return block.match(text, start).captured(1).toUtf8();
    }

    std::unique_ptr<LuaState> newState(const QStringList& permissions)
    {
        PluginManifest manifest;
        manifest.id = "com.example.docs";
        manifest.name = "Docs";
        manifest.permissions = permissions;
        QDir().mkpath(m_dir.filePath("plugin"));
        auto state = std::make_unique<LuaState>();
        if (auto error = PrismApi::install(*state, manifest, m_dir.filePath("plugin"), { &m_host, &m_host, &m_host })) {
            qFatal("install failed: %s", qPrintable(*error));
        }
        return state;
    }

   private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        m_host.root = m_dir.filePath("game");
        QVERIFY(QDir().mkpath(m_host.root + "/saves/World"));
        QFile level(m_host.root + "/saves/World/level.dat");
        QVERIFY(level.open(QIODevice::WriteOnly));
        level.write("level");
    }

    void test_quickStart()
    {
        auto state = newState({ "ui" });
        QTest::ignoreMessage(QtInfoMsg, "[com.example.docs] Hello from my plugin");
        QCOMPARE(state->run(example("4. Add `main.lua`:"), "main.lua"), std::nullopt);
        QCOMPARE(PrismApi::emitEvent(*state, "launcher.ready"), std::nullopt);
        QCOMPARE(m_host.notifications.last(), "The launcher is ready!");
    }

    void test_settings()
    {
        auto state = newState({});
        // The example shows a failing assignment on purpose
        auto error = state->run(example("### Settings"), "settings");
        QVERIFY(error);
        QVERIFY2(error->contains("setting 'interval' must be a number, got string"), qPrintable(*error));
    }

    void test_worldBackup()
    {
        auto manifest = PluginManifest::parse(example("### Back up the worlds before playing", "toml"));
        QVERIFY2(manifest, qPrintable(manifest.error_or("")));
        auto state = newState(manifest->permissions);
        QCOMPARE(state->run(example("### Back up the worlds before playing"), "main.lua"), std::nullopt);

        QList<PrismApi::LaunchStepInfo> steps;
        QCOMPARE(PrismApi::launchSteps(*state, steps), std::nullopt);
        QCOMPARE(steps.size(), 1);
        QCOMPARE(steps.first().anchor, "launch");

        QStringList logged;
        PrismApi::LaunchContext context;
        context.instanceId = "game";
        context.instanceName = "Game";
        context.log = [&logged](const QString& line) { logged.append(line); };
        // Twice: the second run replaces the first backup
        for (int i = 0; i < 2; i++) {
            QCOMPARE(PrismApi::runLaunchStep(*state, steps.first().index, context), std::nullopt);
        }
        QCOMPARE(logged, QStringList({ "Worlds backed up", "Worlds backed up" }));
        QVERIFY(QFile::exists(m_dir.filePath("plugin/data/game/backup/World/level.dat")));
    }

    void test_news()
    {
        auto state = newState({ "net", "ui" });
        QCOMPARE(state->run(example("### Check for news on launch"), "main.lua"), std::nullopt);
        QCOMPARE(PrismApi::emitEvent(*state, "launcher.ready"), std::nullopt);
        QCOMPARE(m_host.requests.last().url, QUrl("https://example.com/news.json"));
        m_host.pending.takeLast()({ 200, "{\"title\": \"Version 12 is out\"}", {}, {} });
        QCOMPARE(m_host.notifications.last(), "Version 12 is out");
    }

    void test_sessions()
    {
        auto state = newState({ "instances.read" });
        QCOMPARE(state->run(example("### Count play sessions"), "main.lua"), std::nullopt);
        QTest::ignoreMessage(QtInfoMsg, "[com.example.docs] Sessions played: 1");
        QCOMPARE(PrismApi::emitEvent(*state, "launch.finished",
                                     { { "id", QByteArray("game") }, { "success", true }, { "reason", QByteArray() } }),
                 std::nullopt);
    }
};

QTEST_GUILESS_MAIN(DocExamplesTest)

#include "DocExamples_test.moc"
