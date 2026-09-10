#include <QTemporaryDir>
#include <QTest>
#include <memory>

#include "BaseInstance.h"
#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "modplatform/flame/FlameInstanceCreationTask.h"
#include "settings/INISettingsObject.h"

class FlameUpdateRootTest : public QObject {
    Q_OBJECT

   private slots:
    void init()
    {
        m_globalDir = std::make_unique<QTemporaryDir>();
        QVERIFY(m_globalDir->isValid());

        auto global = std::make_unique<INISettingsObject>(FS::PathCombine(m_globalDir->path(), "global.cfg"));
        // BaseInstance's constructor requires these to exist in the global settings object.
        global->registerSetting("ShowGameTime", true);
        global->registerSetting("RecordGameTime", true);
        global->registerSetting("PreLaunchCommand", QString());
        global->registerSetting("WrapperCommand", QString());
        global->registerSetting("PostExitCommand", QString());
        global->registerSetting("ShowConsole", false);
        global->registerSetting("AutoCloseConsole", false);
        global->registerSetting("ShowConsoleOnError", false);
        global->registerSetting("LogPrePostOutput", true);
        global->registerSetting("ConsoleMaxLines", 100000);
        global->registerSetting("ConsoleOverflowStop", 100000);

        m_globalSettings = std::move(global);
    }

    void cleanup() { m_globalSettings.reset(); }

    void packRoot_matchesOldDotMinecraftRoot();
    void packRoot_matchesStandardRoot();
    void packRoot_prefersMinecraftWhenBothExist();
    void packRoot_defaultsToMinecraftWhenNeitherExists();

   private:
    std::unique_ptr<MinecraftInstance> makeInstance(const QString& root) const
    {
        auto settings = std::make_unique<INISettingsObject>(FS::PathCombine(root, "instance.cfg"));
        // m_globalSettings must outlive the returned instance; it is owned by the fixture.
        return std::make_unique<MinecraftInstance>(m_globalSettings.get(), std::move(settings), root);
    }

    std::unique_ptr<QTemporaryDir> m_globalDir;
    std::unique_ptr<INISettingsObject> m_globalSettings;
};

// Regression test for #5969: FTB App style instances keep their game files in
// ".minecraft". A CurseForge pack update must reuse that folder instead of
// extracting into a fresh "minecraft" folder, which would orphan the old saves.
void FlameUpdateRootTest::packRoot_matchesOldDotMinecraftRoot()
{
    QTemporaryDir root;
    QVERIFY(QDir(root.path()).mkpath(".minecraft/saves"));

    auto instance = makeInstance(root.path());
    QCOMPARE(FlameCreationTask::packRootForUpdate(instance.get()), QString(".minecraft"));
}

void FlameUpdateRootTest::packRoot_matchesStandardRoot()
{
    QTemporaryDir root;
    QVERIFY(QDir(root.path()).mkpath("minecraft"));

    auto instance = makeInstance(root.path());
    QCOMPARE(FlameCreationTask::packRootForUpdate(instance.get()), QString("minecraft"));
}

// Documents the current gameRoot() tie-break: when both folders exist,
// "minecraft" is the active game root.
void FlameUpdateRootTest::packRoot_prefersMinecraftWhenBothExist()
{
    QTemporaryDir root;
    QVERIFY(QDir(root.path()).mkpath(".minecraft/saves"));
    QVERIFY(QDir(root.path()).mkpath("minecraft"));

    auto instance = makeInstance(root.path());
    QCOMPARE(FlameCreationTask::packRootForUpdate(instance.get()), QString("minecraft"));
}

void FlameUpdateRootTest::packRoot_defaultsToMinecraftWhenNeitherExists()
{
    QTemporaryDir root;

    auto instance = makeInstance(root.path());
    QCOMPARE(FlameCreationTask::packRootForUpdate(instance.get()), QString("minecraft"));
}

QTEST_GUILESS_MAIN(FlameUpdateRootTest)

#include "FlameUpdateRoot_test.moc"
