#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <BaseInstance.h>
#include <launch/LaunchTask.h>
#include <settings/INISettingsObject.h>
#include <tasks/Task.h>

class TestLaunchTask final : public LaunchTask {
   public:
    TestLaunchTask() : LaunchTask(nullptr) {}
};

class TestInstance final : public BaseInstance {
   public:
    TestInstance(SettingsObject* globalSettings, std::unique_ptr<SettingsObject> settings, const QString& rootDir)
        : BaseInstance(globalSettings, std::move(settings), rootDir)
    {}

    void saveNow() override {}
    QString modsRoot() const override { return instanceRoot(); }
    QSet<QString> traits() const override { return {}; }
    void loadSpecificSettings() override { setSpecificSettingsLoaded(true); }
    QList<Task::Ptr> createUpdateTask() override { return {}; }
    LaunchTask* createLaunchTask(AuthSessionPtr, MinecraftTarget::Ptr) override { return nullptr; }
    QProcessEnvironment createEnvironment() override { return {}; }
    QProcessEnvironment createLaunchEnvironment() override { return {}; }
    QStringList getLogFileSearchPaths() override { return {}; }
    QString getStatusbarDescription() override { return {}; }
    QString instanceConfigFolder() const override { return instanceRoot(); }
    QMap<QString, QString> getVariables() override { return {}; }
    bool canEdit() const override { return true; }
    bool canExport() const override { return true; }
    void populateLaunchMenu(QMenu*) override {}
    QStringList verboseDescription(AuthSessionPtr, MinecraftTarget::Ptr) override { return {}; }
};

class BaseInstanceTest : public QObject {
    Q_OBJECT

   private:
    static std::unique_ptr<INISettingsObject> createGlobalSettings(const QString& path)
    {
        auto settings = std::make_unique<INISettingsObject>(path);
        settings->registerSetting("ShowGameTime", true);
        settings->registerSetting("RecordGameTime", true);
        settings->registerSetting("PreLaunchCommand", "");
        settings->registerSetting("WrapperCommand", "");
        settings->registerSetting("PostExitCommand", "");
        settings->registerSetting("ShowConsole", false);
        settings->registerSetting("AutoCloseConsole", false);
        settings->registerSetting("ShowConsoleOnError", true);
        settings->registerSetting("LogPrePostOutput", true);
        settings->registerSetting("ConsoleMaxLines", 1000);
        settings->registerSetting("ConsoleOverflowStop", true);
        return settings;
    }

   private slots:
    void aggregateRunningStateAndCanLaunch()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        auto global = createGlobalSettings(dir.filePath("global.cfg"));
        auto local = std::make_unique<INISettingsObject>(dir.filePath("instance.cfg"));
        TestInstance instance(global.get(), std::move(local), dir.path());
        TestLaunchTask first;
        TestLaunchTask second;
        QSignalSpy runningChanges(&instance, &BaseInstance::runningStatusChanged);

        QVERIFY(instance.canLaunch());
        instance.launchSessionStarted(&first);
        QVERIFY(instance.isRunning());
        QVERIFY(instance.isLaunchTaskActive(&first));
        QVERIFY(instance.canLaunch());
        QCOMPARE(runningChanges.count(), 1);
        QCOMPARE(runningChanges.at(0).at(0).toBool(), true);

        instance.launchSessionStarted(&second);
        QVERIFY(instance.isRunning());
        QVERIFY(instance.isLaunchTaskActive(&second));
        QCOMPARE(runningChanges.count(), 1);

        instance.launchSessionFinished(&first, false);
        QVERIFY(instance.isRunning());
        QVERIFY(!instance.isLaunchTaskActive(&first));
        QVERIFY(instance.isLaunchTaskActive(&second));
        QCOMPARE(runningChanges.count(), 1);

        instance.launchSessionFinished(&second, false);
        QVERIFY(!instance.isRunning());
        QCOMPARE(runningChanges.count(), 2);
        QCOMPARE(runningChanges.at(1).at(0).toBool(), false);
    }

    void parallelMinecraftProcessesCountWallClockTimeOnce()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        auto global = createGlobalSettings(dir.filePath("global.cfg"));
        auto local = std::make_unique<INISettingsObject>(dir.filePath("instance.cfg"));
        TestInstance instance(global.get(), std::move(local), dir.path());
        TestLaunchTask first;
        TestLaunchTask second;

        instance.setMinecraftRunning(&first, true);
        instance.setMinecraftRunning(&second, true);
        QTest::qWait(1100);
        instance.setMinecraftRunning(&first, false);
        QVERIFY(instance.totalTimePlayed() <= 2);
        instance.setMinecraftRunning(&second, false);

        QVERIFY(instance.totalTimePlayed() >= 1);
        QVERIFY(instance.totalTimePlayed() <= 2);
        QCOMPARE(instance.lastTimePlayed(), instance.totalTimePlayed());
    }
};

QTEST_GUILESS_MAIN(BaseInstanceTest)

#include "BaseInstance_test.moc"
