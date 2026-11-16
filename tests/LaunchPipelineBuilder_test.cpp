#include <QCoreApplication>
#include <QLatin1String>
#include <QTemporaryDir>
#include <QTest>

#include <LaunchMode.h>
#include <launch/LaunchPipelineBuilder.h>
#include <launch/LaunchTask.h>
#include <minecraft/MinecraftInstance.h>
#include <minecraft/auth/AuthSession.h>
#include <minecraft/launch/MinecraftTarget.h>
#include <settings/INISettingsObject.h>

#include <memory>

class LaunchPipelineBuilderTest : public QObject {
    Q_OBJECT

   private slots:
    void init();
    void cleanup();

    void offlineDefaultOrder();
    void onlineReplacesOfflineLibraries();
    void optionalCommandsInsertLaunchCommand();
    void defaultPortInsertsLookup();
    void otherPortSkipsLookup();
    void quitAfterGameStopAppendsStep();
    void unknownRecipeAppendsNothing();

   private:
    AuthSessionPtr session(LaunchMode mode) const;
    std::unique_ptr<LaunchTask> applyStandard(AuthSessionPtr authSession,
                                              MinecraftTarget::Ptr target,
                                              const QString& preLoad,
                                              const QString& preLaunch,
                                              const QString& postExit,
                                              bool quitAfterGameStop,
                                              bool* applied);
    static QStringList stepClassNames(const LaunchTask& task);
    static QStringList offlineDefaultSteps();

    std::unique_ptr<QTemporaryDir> m_root;
    std::unique_ptr<INISettingsObject> m_global;
    std::unique_ptr<MinecraftInstance> m_instance;
};

void LaunchPipelineBuilderTest::init()
{
    m_root = std::make_unique<QTemporaryDir>();
    QVERIFY(m_root->isValid());

    m_global = std::make_unique<INISettingsObject>(m_root->filePath("prismlauncher.cfg"));
    const char* globalKeys[] = {
        "ShowGameTime",
        "RecordGameTime",
        "PreLoadCommand",
        "PreLaunchCommand",
        "WrapperCommand",
        "PostExitCommand",
        "ShowConsole",
        "AutoCloseConsole",
        "ShowConsoleOnError",
        "LogPrePostOutput",
        "ConsoleMaxLines",
        "ConsoleOverflowStop",
        "JavaPath",
        "JvmArgs",
        "IgnoreJavaCompatibility",
        "JavaSignature",
        "JavaArchitecture",
        "JavaRealArchitecture",
        "JavaVersion",
        "JavaVendor",
        "LaunchMaximized",
        "MinecraftWinWidth",
        "MinecraftWinHeight",
        "MinMemAlloc",
        "MaxMemAlloc",
        "PermGen",
        "LowMemWarning",
        "UseNativeOpenAL",
        "CustomOpenALPath",
        "UseNativeGLFW",
        "CustomGLFWPath",
        "UseNativeSDL",
        "CustomSDLPath",
        "EnableFeralGamemode",
        "EnableMangoHud",
        "UseDiscreteGpu",
        "UseZink",
        "CloseAfterLaunch",
        "QuitAfterGameStop",
        "OnlineFixes",
        "Env",
    };
    for (const char* key : globalKeys) {
        m_global->registerSetting(QString::fromLatin1(key));
    }

    auto instanceSettings = std::make_unique<INISettingsObject>(m_root->filePath("instance.cfg"));
    m_instance = std::make_unique<MinecraftInstance>(m_global.get(), std::move(instanceSettings), m_root->filePath("instance"));
    m_instance->loadSpecificSettings();
    QCoreApplication::processEvents();
}

void LaunchPipelineBuilderTest::cleanup()
{
    m_instance.reset();
    m_global.reset();
    m_root.reset();
}

AuthSessionPtr LaunchPipelineBuilderTest::session(LaunchMode mode) const
{
    auto authSession = std::make_shared<AuthSession>();
    authSession->launchMode = mode;
    return authSession;
}

std::unique_ptr<LaunchTask> LaunchPipelineBuilderTest::applyStandard(AuthSessionPtr authSession,
                                                                    MinecraftTarget::Ptr target,
                                                                    const QString& preLoad,
                                                                    const QString& preLaunch,
                                                                    const QString& postExit,
                                                                    bool quitAfterGameStop,
                                                                    bool* applied)
{
    auto task = LaunchTask::create(m_instance.get());
    *applied = LaunchPipelineBuilder(task.get())
                   .setSession(std::move(authSession))
                   .setTargetToJoin(std::move(target))
                   .setPreLoadCommand(preLoad)
                   .setPreLaunchCommand(preLaunch)
                   .setPostExitCommand(postExit)
                   .setQuitAfterGameStop(quitAfterGameStop)
                   .apply(QLatin1String(LaunchPipelineBuilder::StandardMinecraftRecipe));
    return task;
}

QStringList LaunchPipelineBuilderTest::stepClassNames(const LaunchTask& task)
{
    QStringList names;
    for (const auto& step : task.steps()) {
        names.append(QLatin1String(step->metaObject()->className()));
    }
    return names;
}

QStringList LaunchPipelineBuilderTest::offlineDefaultSteps()
{
    return { "TextPrint",
             "CreateGameFolders",
             "TaskStepWrapper",
             "AutoInstallJava",
             "CheckJava",
             "VerifyJavaInstall",
             "EnsureOfflineLibraries",
             "ModMinecraftJar",
             "ScanModFolders",
             "EnsureAvailableMemory",
             "PrintInstanceInfo",
             "ExtractNatives",
             "ReconstructAssets",
             "LauncherPartLaunch" };
}

void LaunchPipelineBuilderTest::offlineDefaultOrder()
{
    bool applied = false;
    auto task = applyStandard(session(LaunchMode::Offline), {}, {}, {}, {}, false, &applied);
    QVERIFY(applied);
    QCOMPARE(stepClassNames(*task), offlineDefaultSteps());
}

void LaunchPipelineBuilderTest::onlineReplacesOfflineLibraries()
{
    // Demo still takes the online recipe. Normal would construct ClaimAccount, which looks up APPLICATION->accounts().
    bool applied = false;
    auto task = applyStandard(session(LaunchMode::Demo), {}, {}, {}, {}, false, &applied);
    QVERIFY(applied);

    auto expected = offlineDefaultSteps();
    const int offlineIndex = expected.indexOf(QStringLiteral("EnsureOfflineLibraries"));
    expected.removeAt(offlineIndex);
    expected.insert(offlineIndex, QStringLiteral("ClaimAccount"));
    for (int i = 0; i < 4; ++i) {
        expected.insert(offlineIndex + 1, QStringLiteral("TaskStepWrapper"));
    }
    QCOMPARE(stepClassNames(*task), expected);
}

void LaunchPipelineBuilderTest::optionalCommandsInsertLaunchCommand()
{
    bool applied = false;
    auto task = applyStandard(session(LaunchMode::Offline), {}, "echo preload", "echo prelaunch", "echo postexit", false, &applied);
    QVERIFY(applied);

    auto expected = offlineDefaultSteps();
    expected.insert(expected.indexOf(QStringLiteral("CreateGameFolders")) + 1, QStringLiteral("LaunchCommand"));
    expected.insert(expected.indexOf(QStringLiteral("VerifyJavaInstall")) + 1, QStringLiteral("LaunchCommand"));
    expected.insert(expected.indexOf(QStringLiteral("LauncherPartLaunch")) + 1, QStringLiteral("LaunchCommand"));
    QCOMPARE(stepClassNames(*task), expected);
}

void LaunchPipelineBuilderTest::defaultPortInsertsLookup()
{
    auto target = std::make_shared<MinecraftTarget>();
    target->address = QStringLiteral("example.com");
    target->port = 25565;

    bool applied = false;
    auto task = applyStandard(session(LaunchMode::Offline), target, {}, {}, {}, false, &applied);
    QVERIFY(applied);

    auto expected = offlineDefaultSteps();
    expected.insert(expected.indexOf(QStringLiteral("CreateGameFolders")) + 1, QStringLiteral("LookupServerAddress"));
    QCOMPARE(stepClassNames(*task), expected);
}

void LaunchPipelineBuilderTest::otherPortSkipsLookup()
{
    auto target = std::make_shared<MinecraftTarget>();
    target->address = QStringLiteral("example.com");
    target->port = 25566;

    bool applied = false;
    auto task = applyStandard(session(LaunchMode::Offline), target, {}, {}, {}, false, &applied);
    QVERIFY(applied);
    QCOMPARE(stepClassNames(*task), offlineDefaultSteps());
}

void LaunchPipelineBuilderTest::quitAfterGameStopAppendsStep()
{
    bool applied = false;
    auto task = applyStandard(session(LaunchMode::Offline), {}, {}, {}, {}, true, &applied);
    QVERIFY(applied);

    auto expected = offlineDefaultSteps();
    expected.append(QStringLiteral("QuitAfterGameStop"));
    QCOMPARE(stepClassNames(*task), expected);
}

void LaunchPipelineBuilderTest::unknownRecipeAppendsNothing()
{
    auto task = LaunchTask::create(m_instance.get());
    const bool applied = LaunchPipelineBuilder(task.get()).setSession(session(LaunchMode::Offline)).apply(QStringLiteral("not-a-recipe"));
    QVERIFY(!applied);
    QVERIFY(stepClassNames(*task).isEmpty());
}

QTEST_GUILESS_MAIN(LaunchPipelineBuilderTest)

#include "LaunchPipelineBuilder_test.moc"
