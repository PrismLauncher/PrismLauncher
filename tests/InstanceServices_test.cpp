#include "InstanceServices_test.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>
#include <QVariantMap>

#include <memory>

#include "BuildConfig.h"
#include "minecraft/InstanceServices.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/update/AssetUpdateTask.h"
#include "services/LauncherSettings.h"
#include "settings/INISettingsObject.h"

namespace {

class FakeLauncherSettings : public LauncherSettings {
   public:
    QVariant get(const QString& id) override { return values.value(id); }
    bool set(const QString& id, QVariant value) override
    {
        values[id] = std::move(value);
        return true;
    }
    void reset(const QString& id) override { values.remove(id); }
    bool contains(const QString& id) override { return values.contains(id); }
    std::shared_ptr<Setting> getSetting(const QString&) const override { return nullptr; }
    std::shared_ptr<Setting> getOrRegisterSetting(const QString&, QVariant) override { return nullptr; }
    QVariant getPlaytime(const QString&) override { return {}; }
    bool setPlaytime(const QString&, QVariant) override { return false; }
    SettingsObject* settingsObject() override { return nullptr; }
    SettingsObject* playtimeSettingsObject() override { return nullptr; }

    QVariantMap values;
};

void registerGlobals(INISettingsObject& global)
{
    const char* keys[] = { "ShowGameTime",
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
                           "Env" };
    for (const char* key : keys) {
        global.registerSetting(QString::fromLatin1(key));
    }
}

}  // namespace

void InstanceServicesTest::nativeLibraryPathComesFromInjectedRuntime()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());

    auto global = std::make_unique<INISettingsObject>(root.filePath("prismlauncher.cfg"));
    registerGlobals(*global);
    global->set("UseNativeOpenAL", true);

    QFile library(root.filePath("libopenal.so.1"));
    QVERIFY(library.open(QIODevice::WriteOnly));
    library.write("x");
    library.close();

    LauncherRuntimeInfo runtime;
    runtime.detectedOpenALPath = library.fileName();
    InstanceServices services{ .runtime = &runtime };

    auto instanceSettings = std::make_unique<INISettingsObject>(root.filePath("instance.cfg"));
    MinecraftInstance instance(global.get(), std::move(instanceSettings), root.filePath("instance"), services);
    instance.loadSpecificSettings();
    QCoreApplication::processEvents();

    const QString expected = QFileInfo(library.fileName()).absoluteFilePath();
    QVERIFY(instance.extraArguments().join('\n').contains(expected));
}

void InstanceServicesTest::assetUrlComesFromInjectedSettings()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    auto global = std::make_unique<INISettingsObject>(root.filePath("prismlauncher.cfg"));
    registerGlobals(*global);

    FakeLauncherSettings settings;
    settings.values["ResourceURLOverride"] = "https://example.test/assets/";
    InstanceServices services{ .settings = &settings };

    auto instanceSettings = std::make_unique<INISettingsObject>(root.filePath("instance.cfg"));
    MinecraftInstance instance(global.get(), std::move(instanceSettings), root.filePath("instance"), services);
    instance.loadSpecificSettings();
    QCoreApplication::processEvents();

    AssetUpdateTask task(&instance);
    QCOMPARE(task.resourceUrl(), QString("https://example.test/assets/"));
}

void InstanceServicesTest::missingServicesSkipNativeLibraries()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    auto global = std::make_unique<INISettingsObject>(root.filePath("prismlauncher.cfg"));
    registerGlobals(*global);
    global->set("UseNativeOpenAL", true);

    auto instanceSettings = std::make_unique<INISettingsObject>(root.filePath("instance.cfg"));
    MinecraftInstance instance(global.get(), std::move(instanceSettings), root.filePath("instance"), {});
    instance.loadSpecificSettings();
    QCoreApplication::processEvents();

    QVERIFY(!instance.extraArguments().join(' ').contains("openal"));
    AssetUpdateTask task(&instance);
    QCOMPARE(task.resourceUrl(), BuildConfig.DEFAULT_RESOURCE_BASE);
}

QTEST_GUILESS_MAIN(InstanceServicesTest)
