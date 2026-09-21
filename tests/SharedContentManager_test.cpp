// SPDX-License-Identifier: GPL-3.0-only

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

#include <MMCZip.h>
#include <minecraft/MinecraftInstance.h>
#include <settings/INISettingsObject.h>
#include <shared/SharedContentManager.h>

Q_DECLARE_METATYPE(SharedContent::Category)

class TestMinecraftInstance : public MinecraftInstance {
   public:
    using MinecraftInstance::MinecraftInstance;

    void loadSpecificSettings() override
    {
        if (isSpecificSettingsLoaded()) {
            return;
        }
        m_settings->getOrRegisterSetting("UseLatestMinecraftVersion", false);
        m_settings->getOrRegisterSetting("GlobalDataPacksEnabled", false);
        m_settings->getOrRegisterSetting("GlobalDataPacksPath", QString());
        setSpecificSettingsLoaded(true);
    }
};

class SharedContentManagerTest : public QObject {
    Q_OBJECT

   private slots:
    void groupLifecycle()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());

        const QString root = temporaryDir.filePath("shared content");
        SharedContent::Manager manager(root);
        QCOMPARE(manager.rootPath(), QDir::cleanPath(root));
        QVERIFY(manager.groups().isEmpty());

        QString error;
        QVERIFY2(manager.createGroup("Fabric 1.21", &error), qPrintable(error));
        QVERIFY(QFileInfo(manager.groupPath("Fabric 1.21")).isDir());
        QCOMPARE(manager.groups(), QStringList{ "Fabric 1.21" });

        error.clear();
        QVERIFY2(manager.createGroup("Default", &error), qPrintable(error));
        QCOMPARE(manager.groups(), QStringList({ "Default", "Fabric 1.21" }));

        QFile unrelatedFile(temporaryDir.filePath("shared content/README.txt"));
        QVERIFY(unrelatedFile.open(QIODevice::WriteOnly));
        QCOMPARE(unrelatedFile.write("not a group"), qint64(11));
        unrelatedFile.close();
        QCOMPARE(manager.groups(), QStringList({ "Default", "Fabric 1.21" }));

        QFile marker(temporaryDir.filePath("shared content/Fabric 1.21/config/marker.txt"));
        QVERIFY(QDir().mkpath(QFileInfo(marker).absolutePath()));
        QVERIFY(marker.open(QIODevice::WriteOnly));
        QCOMPARE(marker.write("preserved"), qint64(9));
        marker.close();

        error.clear();
        QVERIFY2(manager.renameGroup("Fabric 1.21", "Fabric", &error), qPrintable(error));
        QVERIFY(!QFileInfo::exists(temporaryDir.filePath("shared content/Fabric 1.21")));
        QVERIFY(QFileInfo::exists(temporaryDir.filePath("shared content/Fabric/config/marker.txt")));
        QCOMPARE(manager.groups(), QStringList({ "Default", "Fabric" }));

        error.clear();
        QVERIFY2(manager.deleteGroup("Default", true, &error), qPrintable(error));
        QVERIFY(!QFileInfo::exists(manager.groupPath("Default")));
        QCOMPARE(manager.groups(), QStringList{ "Fabric" });

        error.clear();
        QVERIFY2(manager.deleteGroup("Fabric", true, &error), qPrintable(error));
        QVERIFY(manager.groups().isEmpty());
    }

    void groupLifecycleRejectsInvalidOperations()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        SharedContent::Manager manager(temporaryDir.filePath("shared"));

        QString error;
        QVERIFY2(manager.createGroup("one", &error), qPrintable(error));
        error.clear();
        QVERIFY2(manager.createGroup("two", &error), qPrintable(error));

        error.clear();
        QVERIFY(!manager.createGroup("one", &error));
        QVERIFY(!error.isEmpty());

        error.clear();
        QVERIFY(!manager.renameGroup("missing", "renamed", &error));
        QVERIFY(!error.isEmpty());

        error.clear();
        QVERIFY(!manager.renameGroup("one", "two", &error));
        QVERIFY(!error.isEmpty());

        error.clear();
        QVERIFY2(manager.deleteGroup("missing", true, &error), qPrintable(error));

        QFile content(manager.groupPath("one") + "/minecraft/content.txt");
        QVERIFY(content.open(QIODevice::WriteOnly));
        content.close();
        error.clear();
        QVERIFY(!manager.deleteGroup("one", false, &error));
        QVERIFY(!error.isEmpty());

        QCOMPARE(manager.groups(), QStringList({ "one", "two" }));
    }

    void groupNameValidation_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<bool>("valid");

        QTest::newRow("simple") << QString("Default") << true;
        QTest::newRow("spaces") << QString("Fabric 1.21") << true;
        QTest::newRow("unicode") << QString::fromUtf8("共有") << true;
        QTest::newRow("empty") << QString() << false;
        QTest::newRow("whitespace") << QString("   ") << false;
        QTest::newRow("leading whitespace") << QString(" Default") << false;
        QTest::newRow("trailing whitespace") << QString("Default ") << false;
        QTest::newRow("dot") << QString(".") << false;
        QTest::newRow("dot dot") << QString("..") << false;
        QTest::newRow("forward slash") << QString("parent/child") << false;
        QTest::newRow("backslash") << QString("parent\\child") << false;
        QTest::newRow("absolute unix path") << QString("/tmp/shared") << false;
        QTest::newRow("absolute windows path") << QString("C:/shared") << false;
        QTest::newRow("newline") << QString("first\nsecond") << false;
    }

    void groupNameValidation()
    {
        QFETCH(QString, name);
        QFETCH(bool, valid);

        QString error;
        QCOMPARE(SharedContent::Manager::isValidGroupName(name, &error), valid);
        if (!valid) {
            QVERIFY(!error.isEmpty());
        }
    }

    void categoryIds_data()
    {
        QTest::addColumn<SharedContent::Category>("category");
        QTest::addColumn<QString>("id");

        using SharedContent::Category;
        QTest::newRow("none") << Category::None << QString();
        QTest::newRow("options") << Category::Options << QString("options");
        QTest::newRow("screenshots") << Category::Screenshots << QString("screenshots");
        QTest::newRow("resource packs") << Category::ResourcePacks << QString("resourcepacks");
        QTest::newRow("texture packs") << Category::TexturePacks << QString("texturepacks");
        QTest::newRow("shader packs") << Category::ShaderPacks << QString("shaderpacks");
        QTest::newRow("config") << Category::Config << QString("config");
        QTest::newRow("servers") << Category::Servers << QString("servers");
        QTest::newRow("command history") << Category::CommandHistory << QString("command_history");
        QTest::newRow("creative hotbar") << Category::CreativeHotbar << QString("creative_hotbar");
        QTest::newRow("global data packs") << Category::GlobalDataPacks << QString("global_datapacks");
    }

    void categoryIds()
    {
        QFETCH(SharedContent::Category, category);
        QFETCH(QString, id);

        QCOMPARE(SharedContent::Manager::categoryId(category), id);
        QCOMPARE(SharedContent::Manager::categoryFromId(id), category);
    }

    void categorySerialization()
    {
        using SharedContent::Categories;
        using SharedContent::Category;

        const Categories selected = Category::Options | Category::Screenshots | Category::ShaderPacks | Category::Config |
                                    Category::Servers | Category::CreativeHotbar | Category::GlobalDataPacks;
        const QStringList expected{ "options", "screenshots", "shaderpacks", "config", "servers", "creative_hotbar", "global_datapacks" };

        QCOMPARE(SharedContent::Manager::serializeCategories(selected), expected);
        QCOMPARE(SharedContent::Manager::deserializeCategories(expected), selected);
        QCOMPARE(SharedContent::Manager::serializeCategories({}), QStringList());
        QCOMPARE(SharedContent::Manager::deserializeCategories({}), Categories());

        QCOMPARE(SharedContent::Manager::deserializeCategories({ "options", "future-category", "options", "config" }),
                 Categories(Category::Options | Category::Config));
        QCOMPARE(SharedContent::Manager::categoryFromId("future-category"), Category::None);
    }

    void customPathSerialization()
    {
        using SharedContent::CustomPath;

        QCOMPARE(SharedContent::Manager::serializeCustomPath({ "schematics", true }), QString("dir:schematics"));
        QCOMPARE(SharedContent::Manager::serializeCustomPath({ "servers.dat", false }), QString("file:servers.dat"));
        QCOMPARE(SharedContent::Manager::serializeCustomPath({ "nested/folder", true }), QString("dir:nested/folder"));

        const CustomPath directory = SharedContent::Manager::deserializeCustomPath("dir:schematics");
        QCOMPARE(directory.relativePath, QString("schematics"));
        QVERIFY(directory.directory);

        const CustomPath file = SharedContent::Manager::deserializeCustomPath("file:journeymap/data.json");
        QCOMPARE(file.relativePath, QString("journeymap/data.json"));
        QVERIFY(!file.directory);

        const CustomPath legacyDirectory = SharedContent::Manager::deserializeCustomPath("nested/folder");
        QCOMPARE(legacyDirectory.relativePath, QString("nested/folder"));
        QVERIFY(legacyDirectory.directory);

        const CustomPath mixedCasePrefix = SharedContent::Manager::deserializeCustomPath("FiLe:data.bin");
        QCOMPARE(mixedCasePrefix.relativePath, QString("data.bin"));
        QVERIFY(!mixedCasePrefix.directory);
    }

    void customPathValidation_data()
    {
        QTest::addColumn<QString>("path");
        QTest::addColumn<bool>("valid");

        QTest::newRow("directory") << QString("schematics") << true;
        QTest::newRow("nested directory") << QString("journeymap/data") << true;
        QTest::newRow("file") << QString("servers-custom.dat") << true;
        QTest::newRow("unicode") << QString::fromUtf8("データ/設定.dat") << true;

        QTest::newRow("empty") << QString() << false;
        QTest::newRow("dot") << QString(".") << false;
        QTest::newRow("dot dot") << QString("..") << false;
        QTest::newRow("parent traversal") << QString("../outside") << false;
        QTest::newRow("nested traversal") << QString("config/../../outside") << false;
        QTest::newRow("backslash traversal") << QString("config\\..\\..\\outside") << false;
        QTest::newRow("absolute unix path") << QString("/tmp/outside") << false;
        QTest::newRow("absolute windows path") << QString("C:/outside") << false;
        QTest::newRow("noncanonical backslash") << QString("journeymap\\data") << false;
        QTest::newRow("trailing separator") << QString("journeymap/") << false;

        QTest::newRow("mods") << QString("mods") << false;
        QTest::newRow("mods child") << QString("mods/client") << false;
        QTest::newRow("mods case insensitive") << QString("MoDs/client") << false;
        QTest::newRow("coremods") << QString("coremods") << false;
        QTest::newRow("coremods child") << QString("coremods/legacy") << false;
        QTest::newRow("nilmods") << QString("nilmods") << false;
        QTest::newRow("nilmods child") << QString("nilmods/generated") << false;
        QTest::newRow("saves") << QString("saves") << false;
        QTest::newRow("saves child") << QString("saves/world") << false;
        QTest::newRow("backslash blocked child") << QString("saves\\world") << false;

        QTest::newRow("mods suffix") << QString("mods-backup") << true;
        QTest::newRow("saves suffix") << QString("saves-old/world") << true;
    }

    void customPathValidation()
    {
        QFETCH(QString, path);
        QFETCH(bool, valid);

        QString error;
        QCOMPARE(SharedContent::Manager::validateCustomPath(path, &error), valid);
        if (!valid) {
            QVERIFY(!error.isEmpty());
        }
    }

    void rootAndGroupPaths()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());

        const QString initialRoot = temporaryDir.filePath("first/../shared");
        SharedContent::Manager manager(initialRoot);
        QCOMPARE(manager.rootPath(), QDir::cleanPath(initialRoot));
        QCOMPARE(manager.groupPath("Default"), QDir(manager.rootPath()).filePath("Default"));

        QString error;
        const QString secondRoot = temporaryDir.filePath("elsewhere/shared-root");
        QVERIFY2(manager.setRootPath(secondRoot, &error), qPrintable(error));
        QCOMPARE(manager.rootPath(), QDir::cleanPath(secondRoot));
        QVERIFY(QFileInfo(manager.rootPath()).isDir());
        QCOMPARE(manager.groupPath("Fabric 1.21"), QDir(manager.rootPath()).filePath("Fabric 1.21"));

        const QString filePath = temporaryDir.filePath("ordinary-file");
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        error.clear();
        QVERIFY(!manager.setRootPath(filePath, &error));
        QVERIFY(!error.isEmpty());
        QCOMPARE(manager.rootPath(), QDir::cleanPath(secondRoot));
    }

    void directoryMigrationAndDisconnect()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        auto global = makeGlobalSettings(temporaryDir.filePath("global.cfg"));
        auto instance = makeInstance(global.get(), temporaryDir.filePath("instance"));
        SharedContent::Manager manager(temporaryDir.filePath("shared"));
        QString error;
        QVERIFY2(manager.createGroup("Default", &error), qPrintable(error));

        const QString localScreenshots = instance->gameRoot() + "/screenshots";
        const QString sharedScreenshots = manager.groupPath("Default") + "/minecraft/screenshots";
        QVERIFY(write(localScreenshots + "/local.png", "local"));
        QVERIFY(write(sharedScreenshots + "/shared.png", "shared"));
        QVERIFY(write(sharedScreenshots + "/conflict.png", "shared wins"));
        QVERIFY(write(localScreenshots + "/conflict.png", "local loses"));

        QVERIFY2(manager.configureInstance(instance.get(), "Default", SharedContent::Category::Screenshots, {}, {},
                                           SharedContent::MigrationPolicy::PreferShared, &error),
                 qPrintable(error));
        QVERIFY(QFileInfo(localScreenshots).isSymLink());
        QCOMPARE(read(localScreenshots + "/local.png"), QByteArray("local"));
        QCOMPARE(read(localScreenshots + "/shared.png"), QByteArray("shared"));
        QCOMPARE(read(localScreenshots + "/conflict.png"), QByteArray("shared wins"));
        QVERIFY(QFileInfo::exists(instance->instanceRoot() + "/.prism-shared-backup"));

        QVERIFY(write(sharedScreenshots + "/later.png", "updated later"));

        QVERIFY2(manager.disconnectInstance(instance.get(), true, &error), qPrintable(error));
        QVERIFY(!QFileInfo(localScreenshots).isSymLink());
        QCOMPARE(read(localScreenshots + "/shared.png"), QByteArray("shared"));
        QCOMPARE(read(localScreenshots + "/later.png"), QByteArray("updated later"));
        QVERIFY(QFileInfo::exists(sharedScreenshots + "/local.png"));
        QVERIFY(manager.instanceGroup(instance.get()).isEmpty());
    }

    void optionsAndFileLaunchLifecycle()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        auto global = makeGlobalSettings(temporaryDir.filePath("global.cfg"));
        auto instance = makeInstance(global.get(), temporaryDir.filePath("instance"));
        SharedContent::Manager manager(temporaryDir.filePath("shared"));
        QString error;
        QVERIFY2(manager.createGroup("Default", &error), qPrintable(error));

        const QString game = instance->gameRoot();
        const QString shared = manager.groupPath("Default") + "/minecraft";
        QVERIFY(write(game + "/options.txt", "gamma:0.5\nguiScale:2\nlocalOnly:yes\n"));
        QVERIFY(write(shared + "/options.txt", "gamma:1.0\nguiScale:4\nsharedOnly:yes\n"));
        QVERIFY(write(game + "/servers.dat", "local servers"));
        QVERIFY(write(shared + "/servers.dat", "shared servers"));

        const auto categories = SharedContent::Category::Options | SharedContent::Category::Servers |
                                SharedContent::Category::CommandHistory | SharedContent::Category::CreativeHotbar;
        QVERIFY2(manager.configureInstance(instance.get(), "Default", categories, {}, { "guiScale" },
                                           SharedContent::MigrationPolicy::PreferShared, &error),
                 qPrintable(error));
        QCOMPARE(read(game + "/options.txt"), QByteArray("gamma:1.0\nguiScale:2\nlocalOnly:yes\nsharedOnly:yes\n"));
        QCOMPARE(read(game + "/servers.dat"), QByteArray("shared servers"));

        QVERIFY2(manager.prepare(instance.get(), &error), qPrintable(error));
        QVERIFY(write(game + "/options.txt", "gamma:0.75\nguiScale:3\nlocalOnly:yes\n"));
        QVERIFY(write(game + "/servers.dat", "changed servers"));
        QVERIFY(write(game + "/command_history.txt", "last command"));
        QVERIFY(write(game + "/hotbar.nbt", "hotbar"));
        QVERIFY2(manager.finish(instance.get(), &error), qPrintable(error));
        QCOMPARE(read(shared + "/options.txt"), QByteArray("gamma:0.75\nguiScale:4\nsharedOnly:yes\nlocalOnly:yes\n"));
        QCOMPARE(read(shared + "/servers.dat"), QByteArray("changed servers"));
        QCOMPARE(read(shared + "/command_history.txt"), QByteArray("last command"));
        QCOMPARE(read(shared + "/hotbar.nbt"), QByteArray("hotbar"));

        QVERIFY(write(shared + "/servers.dat", "changed by another instance"));
        QVERIFY2(manager.disconnectInstance(instance.get(), true, &error), qPrintable(error));
        QCOMPARE(read(game + "/servers.dat"), QByteArray("changed by another instance"));
        QCOMPARE(read(game + "/options.txt"), QByteArray("gamma:0.75\nguiScale:3\nlocalOnly:yes\nsharedOnly:yes\n"));
    }

    void switchGroupsAndCustomDirectories()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        auto global = makeGlobalSettings(temporaryDir.filePath("global.cfg"));
        auto instance = makeInstance(global.get(), temporaryDir.filePath("instance"));
        SharedContent::Manager manager(temporaryDir.filePath("shared"));
        QString error;
        QVERIFY2(manager.createGroup("A", &error), qPrintable(error));
        QVERIFY2(manager.createGroup("B", &error), qPrintable(error));

        const auto game = instance->gameRoot();
        QVERIFY(write(game + "/screenshots/local.png", "local"));
        QVERIFY(write(game + "/schematics/items/a.nbt", "schematic"));
        const QList<SharedContent::CustomPath> paths{ { "schematics", true } };
        QVERIFY2(manager.configureInstance(instance.get(), "A", SharedContent::Category::Screenshots, paths, {},
                                           SharedContent::MigrationPolicy::PreferShared, &error),
                 qPrintable(error));
        QVERIFY(QFileInfo(game + "/schematics").isSymLink());
        QVERIFY(write(manager.groupPath("B") + "/minecraft/screenshots/b.png", "from B"));
        QVERIFY2(manager.configureInstance(instance.get(), "B", SharedContent::Category::Screenshots, paths, {},
                                           SharedContent::MigrationPolicy::PreferShared, &error),
                 qPrintable(error));
        QCOMPARE(manager.instanceGroup(instance.get()), QString("B"));
        QCOMPARE(read(game + "/screenshots/local.png"), QByteArray("local"));
        QCOMPARE(read(game + "/screenshots/b.png"), QByteArray("from B"));
        QCOMPARE(read(game + "/schematics/items/a.nbt"), QByteArray("schematic"));
        QVERIFY(QFileInfo(game + "/schematics").isSymLink());
    }

    void renameGroupRepairsMemberLinks()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        auto global = makeGlobalSettings(temporaryDir.filePath("global.cfg"));
        auto instance = makeInstance(global.get(), temporaryDir.filePath("instance"));
        SharedContent::Manager manager(temporaryDir.filePath("shared"));
        QString error;
        QVERIFY2(manager.createGroup("Old", &error), qPrintable(error));
        QVERIFY(write(instance->gameRoot() + "/screenshots/one.png", "one"));
        QVERIFY2(manager.configureInstance(instance.get(), "Old", SharedContent::Category::Screenshots, {}, {},
                                           SharedContent::MigrationPolicy::PreferShared, &error),
                 qPrintable(error));
        QVERIFY2(manager.renameGroup("Old", "New", &error), qPrintable(error));
        QVERIFY2(manager.configureInstance(instance.get(), "New", SharedContent::Category::Screenshots, {}, {},
                                           SharedContent::MigrationPolicy::PreferShared, &error),
                 qPrintable(error));
        QCOMPARE(manager.instanceGroup(instance.get()), QString("New"));
        QCOMPARE(read(instance->gameRoot() + "/screenshots/one.png"), QByteArray("one"));
        QCOMPARE(QFileInfo(instance->gameRoot() + "/screenshots").symLinkTarget(), manager.groupPath("New") + "/minecraft/screenshots");
    }

    void mutableGroupsAllowOnlyOneRunningInstance()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        auto global = makeGlobalSettings(temporaryDir.filePath("global.cfg"));
        auto first = makeInstance(global.get(), temporaryDir.filePath("first"));
        auto second = makeInstance(global.get(), temporaryDir.filePath("second"));
        SharedContent::Manager manager(temporaryDir.filePath("shared"));
        QString error;
        QVERIFY2(manager.createGroup("Default", &error), qPrintable(error));
        for (auto* instance : { first.get(), second.get() }) {
            QVERIFY2(manager.configureInstance(instance, "Default", SharedContent::Category::Config, {}, {},
                                               SharedContent::MigrationPolicy::PreferShared, &error),
                     qPrintable(error));
        }
        QVERIFY2(manager.prepare(first.get(), &error), qPrintable(error));
        error.clear();
        QVERIFY(!manager.prepare(second.get(), &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!manager.disconnectInstance(second.get(), true, &error));
        QVERIFY2(manager.finish(first.get(), &error), qPrintable(error));
        QVERIFY2(manager.prepare(second.get(), &error), qPrintable(error));
        manager.cancel(second.get());
    }

    void rejectsOverlappingPathsAndProtectedDataPackLocation()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        auto global = makeGlobalSettings(temporaryDir.filePath("global.cfg"));
        auto instance = makeInstance(global.get(), temporaryDir.filePath("instance"));
        SharedContent::Manager manager(temporaryDir.filePath("shared"));
        QString error;
        QVERIFY2(manager.createGroup("Default", &error), qPrintable(error));
        QVERIFY(!manager.configureInstance(instance.get(), "Default", {}, { { "maps", true }, { "maps/sub", true } }, {},
                                           SharedContent::MigrationPolicy::PreferShared, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(manager.instanceGroup(instance.get()).isEmpty());

        instance->settings()->set("GlobalDataPacksPath", "saves");
        error.clear();
        QVERIFY(!manager.configureInstance(instance.get(), "Default", SharedContent::Category::GlobalDataPacks, {}, {},
                                           SharedContent::MigrationPolicy::PreferShared, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(manager.instanceGroup(instance.get()).isEmpty());
        QVERIFY(!QFileInfo(instance->gameRoot() + "/saves").isSymLink());

        instance->settings()->set("GlobalDataPacksPath", "schematics");
        error.clear();
        QVERIFY(!manager.configureInstance(instance.get(), "Default", SharedContent::Category::GlobalDataPacks, { { "schematics", true } },
                                           {}, SharedContent::MigrationPolicy::PreferShared, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(manager.instanceGroup(instance.get()).isEmpty());
        QVERIFY(!QFileInfo(instance->gameRoot() + "/schematics").isSymLink());
    }

    void rejectsChangedSharedDataPackPathWithoutTouchingMods()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        auto global = makeGlobalSettings(temporaryDir.filePath("global.cfg"));
        auto instance = makeInstance(global.get(), temporaryDir.filePath("instance"));
        SharedContent::Manager manager(temporaryDir.filePath("shared"));
        QString error;
        QVERIFY2(manager.createGroup("Default", &error), qPrintable(error));
        QVERIFY(write(instance->gameRoot() + "/mods/example.jar", "instance mod"));
        QVERIFY2(manager.configureInstance(instance.get(), "Default", SharedContent::Category::GlobalDataPacks, {}, {},
                                           SharedContent::MigrationPolicy::PreferShared, &error),
                 qPrintable(error));
        QVERIFY(QFileInfo(instance->gameRoot() + "/datapacks").isSymLink());

        instance->settings()->set("GlobalDataPacksPath", "mods");
        error.clear();
        QVERIFY(!manager.prepare(instance.get(), &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!QFileInfo(instance->gameRoot() + "/mods").isSymLink());
        QCOMPARE(read(instance->gameRoot() + "/mods/example.jar"), QByteArray("instance mod"));

        error.clear();
        QVERIFY2(manager.disconnectInstance(instance.get(), true, &error), qPrintable(error));
        QVERIFY(!QFileInfo(instance->gameRoot() + "/datapacks").isSymLink());
        QVERIFY(!QFileInfo(instance->gameRoot() + "/mods").isSymLink());
    }

    void rejectsChangedDataPackLocationWithSameSharedTarget()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        auto global = makeGlobalSettings(temporaryDir.filePath("global.cfg"));
        auto instance = makeInstance(global.get(), temporaryDir.filePath("instance"));
        SharedContent::Manager manager(temporaryDir.filePath("shared"));
        QString error;
        QVERIFY2(manager.createGroup("Default", &error), qPrintable(error));
        QVERIFY2(manager.configureInstance(instance.get(), "Default", SharedContent::Category::GlobalDataPacks, {}, {},
                                           SharedContent::MigrationPolicy::PreferShared, &error),
                 qPrintable(error));

        const QString original = instance->dataPacksDir();
        const QString alternate = instance->gameRoot() + "/otherpacks";
        const QString shared = manager.groupPath("Default") + "/minecraft/datapacks";
        if (!QFile::link(shared, alternate)) {
            QSKIP("Creating a test symlink is unavailable on this platform");
        }
        instance->settings()->set("GlobalDataPacksPath", "otherpacks");

        error.clear();
        QVERIFY(!manager.prepare(instance.get(), &error));
        QVERIFY(!error.isEmpty());
        error.clear();
        QVERIFY(!manager.configureInstance(instance.get(), "Default", SharedContent::Category::GlobalDataPacks, {}, {},
                                           SharedContent::MigrationPolicy::PreferShared, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(QFileInfo(original).isSymLink());
        QVERIFY(QFileInfo(alternate).isSymLink());
        QCOMPARE(manager.instanceGroup(instance.get()), QString("Default"));
    }

    void renamedInstanceRetainsSharedDataPacks_data()
    {
        QTest::addColumn<QString>("dataPacksPath");
        QTest::addColumn<bool>("legacyAbsolute");

        QTest::newRow("default folder") << QString() << false;
        QTest::newRow("custom folder") << QString("extras/packs") << false;
        QTest::newRow("legacy default folder") << QString() << true;
        QTest::newRow("legacy custom folder") << QString("extras/packs") << true;
    }

    void renamedInstanceRetainsSharedDataPacks()
    {
        QFETCH(QString, dataPacksPath);
        QFETCH(bool, legacyAbsolute);

        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        auto global = makeGlobalSettings(temporaryDir.filePath("global.cfg"));
        const QString oldRoot = temporaryDir.filePath("26.2");
        const QString newRoot = temporaryDir.filePath("26.3");
        auto instance = makeInstance(global.get(), oldRoot);
        SharedContent::Manager manager(temporaryDir.filePath("shared"));
        QString error;
        QVERIFY2(manager.createGroup("Default", &error), qPrintable(error));

        instance->settings()->set("GlobalDataPacksPath", dataPacksPath);
        const QString oldDataPacksPath = QDir::cleanPath(instance->dataPacksDir());
        QVERIFY(write(oldDataPacksPath + "/local.zip", "local data pack"));
        QVERIFY(write(instance->gameRoot() + "/mods/example.jar", "instance mod"));
        QVERIFY2(manager.configureInstance(instance.get(), "Default", SharedContent::Category::GlobalDataPacks, {}, {},
                                           SharedContent::MigrationPolicy::PreferShared, &error),
                 qPrintable(error));
        QVERIFY(QFileInfo(oldDataPacksPath).isSymLink());
        const QString storedDataPacksPath = instance->settings()->get("SharedContentDataPacksPath").toString();
        QVERIFY(!QDir::isAbsolutePath(storedDataPacksPath));
        QCOMPARE(QDir::cleanPath(QDir(instance->gameRoot()).filePath(storedDataPacksPath)), oldDataPacksPath);
        if (legacyAbsolute) {
            instance->settings()->set("SharedContentDataPacksPath", oldDataPacksPath);
        }

        instance.reset();
        QVERIFY(QFile::rename(oldRoot, newRoot));
        instance = makeInstance(global.get(), newRoot);
        QCOMPARE(manager.instanceGroup(instance.get()), QString("Default"));
        const QString newDataPacksPath = QDir::cleanPath(instance->dataPacksDir());
        QVERIFY(QFileInfo(newDataPacksPath).isSymLink());
        QCOMPARE(read(newDataPacksPath + "/local.zip"), QByteArray("local data pack"));

        instance->settings()->set("GlobalDataPacksPath", "mods");
        error.clear();
        QVERIFY(!manager.prepare(instance.get(), &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!QFileInfo(instance->gameRoot() + "/mods").isSymLink());
        QCOMPARE(read(instance->gameRoot() + "/mods/example.jar"), QByteArray("instance mod"));

        instance->settings()->set("GlobalDataPacksPath", dataPacksPath);
        error.clear();
        QVERIFY2(manager.prepare(instance.get(), &error), qPrintable(error));
        const QString migratedPath = instance->settings()->get("SharedContentDataPacksPath").toString();
        QVERIFY(!QDir::isAbsolutePath(migratedPath));
        QCOMPARE(QDir::cleanPath(QDir(instance->gameRoot()).filePath(migratedPath)), newDataPacksPath);
        QVERIFY2(manager.finish(instance.get(), &error), qPrintable(error));

        error.clear();
        QVERIFY2(manager.disconnectInstance(instance.get(), true, &error), qPrintable(error));
        QVERIFY(!QFileInfo(newDataPacksPath).isSymLink());
        QCOMPARE(read(newDataPacksPath + "/local.zip"), QByteArray("local data pack"));
        QCOMPARE(read(instance->gameRoot() + "/mods/example.jar"), QByteArray("instance mod"));
        QVERIFY(manager.instanceGroup(instance.get()).isEmpty());
    }

    void disconnectRejectsLegacyDataPackPathOutsideInstance()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        auto global = makeGlobalSettings(temporaryDir.filePath("global.cfg"));
        auto instance = makeInstance(global.get(), temporaryDir.filePath("instance"));
        SharedContent::Manager manager(temporaryDir.filePath("shared"));
        QString error;
        QVERIFY2(manager.createGroup("Default", &error), qPrintable(error));
        QVERIFY2(manager.configureInstance(instance.get(), "Default", SharedContent::Category::GlobalDataPacks, {}, {},
                                           SharedContent::MigrationPolicy::PreferShared, &error),
                 qPrintable(error));

        const QString local = instance->dataPacksDir();
        const QString shared = manager.groupPath("Default") + "/minecraft/datapacks";
        const QString outside = temporaryDir.filePath("outside/datapacks");
        QVERIFY(QDir().mkpath(QFileInfo(outside).absolutePath()));
        if (!QFile::link(shared, outside)) {
            QSKIP("Creating a test symlink is unavailable on this platform");
        }
        QVERIFY(QFileInfo(local).isSymLink());
        QVERIFY(QFileInfo(outside).isSymLink());
        instance->settings()->set("SharedContentDataPacksPath", outside);

        error.clear();
        QVERIFY(!manager.disconnectInstance(instance.get(), true, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(QFileInfo(outside).isSymLink());
        QCOMPARE(QFileInfo(outside).symLinkTarget(), shared);
        QVERIFY(QFileInfo(local).isSymLink());
        QCOMPARE(manager.instanceGroup(instance.get()), QString("Default"));
    }

    void otherManagerCannotModifyActiveGroup()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        auto global = makeGlobalSettings(temporaryDir.filePath("global.cfg"));
        auto first = makeInstance(global.get(), temporaryDir.filePath("first"));
        auto second = makeInstance(global.get(), temporaryDir.filePath("second"));
        const QString root = temporaryDir.filePath("shared");
        SharedContent::Manager launcherOne(root);
        SharedContent::Manager launcherTwo(root);
        QString error;
        QVERIFY2(launcherOne.createGroup("Default", &error), qPrintable(error));
        QVERIFY2(launcherOne.configureInstance(first.get(), "Default", SharedContent::Category::Screenshots, {}, {},
                                               SharedContent::MigrationPolicy::PreferShared, &error),
                 qPrintable(error));
        QVERIFY2(launcherTwo.configureInstance(second.get(), "Default", SharedContent::Category::Screenshots, {}, {},
                                               SharedContent::MigrationPolicy::PreferShared, &error),
                 qPrintable(error));
        QVERIFY2(launcherOne.prepare(first.get(), &error), qPrintable(error));

        error.clear();
        QVERIFY(!launcherTwo.renameGroup("Default", "Renamed", &error));
        QVERIFY(!error.isEmpty());
        error.clear();
        QVERIFY(!launcherTwo.deleteGroup("Default", true, &error));
        QVERIFY(!error.isEmpty());
        error.clear();
        QVERIFY(!launcherTwo.repairInstance(second.get(), &error));
        QVERIFY(!error.isEmpty());
        error.clear();
        QVERIFY(!launcherTwo.configureInstance(second.get(), "Default", SharedContent::Category::Screenshots, {}, {},
                                               SharedContent::MigrationPolicy::PreferShared, &error));
        QVERIFY(!error.isEmpty());
        error.clear();
        QVERIFY(!launcherTwo.disconnectInstance(first.get(), true, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(QFileInfo(launcherOne.groupPath("Default")).isDir());

        launcherOne.cancel(first.get());
        error.clear();
        QVERIFY2(launcherTwo.renameGroup("Default", "Renamed", &error), qPrintable(error));
    }

    void rejectsSharedRootNestedInInstanceFolder()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        auto global = makeGlobalSettings(temporaryDir.filePath("global.cfg"));
        auto instance = makeInstance(global.get(), temporaryDir.filePath("instance"));
        const QString screenshots = instance->gameRoot() + "/screenshots";
        QVERIFY(write(screenshots + "/local.png", "local"));
        SharedContent::Manager manager(screenshots + "/shared");
        QString error;
        QVERIFY2(manager.createGroup("Default", &error), qPrintable(error));
        error.clear();
        QVERIFY(!manager.configureInstance(instance.get(), "Default", SharedContent::Category::Screenshots, {}, {},
                                           SharedContent::MigrationPolicy::PreferShared, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!QFileInfo(screenshots).isSymLink());
        QCOMPARE(read(screenshots + "/local.png"), QByteArray("local"));
        QVERIFY(QFileInfo(manager.groupPath("Default")).isDir());
    }

    void customPathsNeverEscapeThroughSymlinkParents()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        auto global = makeGlobalSettings(temporaryDir.filePath("global.cfg"));
        auto instance = makeInstance(global.get(), temporaryDir.filePath("instance"));
        SharedContent::Manager manager(temporaryDir.filePath("shared"));
        QString error;
        QVERIFY2(manager.createGroup("Default", &error), qPrintable(error));
        QVERIFY(QDir().mkpath(instance->gameRoot()));
        QVERIFY(QFile::link(temporaryDir.path(), instance->gameRoot() + "/outside"));

        QVERIFY(!manager.configureInstance(instance.get(), "Default", {}, { { "outside/data", true } }, {},
                                           SharedContent::MigrationPolicy::PreferShared, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(manager.instanceGroup(instance.get()).isEmpty());
        QVERIFY(!QFileInfo::exists(temporaryDir.filePath("data")));
    }

    void nestedLinksAreNotImportedIntoSharedFolders()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        auto global = makeGlobalSettings(temporaryDir.filePath("global.cfg"));
        auto instance = makeInstance(global.get(), temporaryDir.filePath("instance"));
        SharedContent::Manager manager(temporaryDir.filePath("shared"));
        QString error;
        QVERIFY2(manager.createGroup("Default", &error), qPrintable(error));
        const auto local = instance->gameRoot() + "/screenshots";
        QVERIFY(QDir().mkpath(local));
        const auto outside = temporaryDir.filePath("outside.txt");
        QVERIFY(write(outside, "outside"));
        if (!QFile::link(outside, local + "/linked.txt")) {
            QSKIP("Creating a test symlink is unavailable on this platform");
        }
        QVERIFY(!manager.configureInstance(instance.get(), "Default", SharedContent::Category::Screenshots, {}, {},
                                           SharedContent::MigrationPolicy::PreferInstance, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!QFileInfo(local).isSymLink());
        QCOMPARE(read(outside), QByteArray("outside"));
    }

    void exportCollectorIncludesLinkedSharedContents()
    {
        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        auto global = makeGlobalSettings(temporaryDir.filePath("global.cfg"));
        auto instance = makeInstance(global.get(), temporaryDir.filePath("instance"));
        SharedContent::Manager manager(temporaryDir.filePath("shared"));
        QString error;
        QVERIFY2(manager.createGroup("Default", &error), qPrintable(error));
        QVERIFY(write(manager.groupPath("Default") + "/minecraft/screenshots/snap.png", "snapshot"));
        QVERIFY2(manager.configureInstance(instance.get(), "Default", SharedContent::Category::Screenshots, {}, {},
                                           SharedContent::MigrationPolicy::PreferShared, &error),
                 qPrintable(error));
        QFileInfoList files;
        QVERIFY(MMCZip::collectFileListRecursively(instance->instanceRoot(), nullptr, &files, nullptr));
        bool found = false;
        for (const auto& file : files) {
            if (file.fileName() == QStringLiteral("snap.png")) {
                found = true;
                QCOMPARE(read(file.absoluteFilePath()), QByteArray("snapshot"));
            }
        }
        QVERIFY(found);
    }

   private:
    static std::unique_ptr<INISettingsObject> makeGlobalSettings(const QString& path)
    {
        auto settings = std::make_unique<INISettingsObject>(path);
        for (const auto& key :
             { "ShowGameTime", "RecordGameTime", "PreLoadCommand", "PreLaunchCommand", "WrapperCommand", "PostExitCommand", "ShowConsole",
               "AutoCloseConsole", "ShowConsoleOnError", "LogPrePostOutput", "ConsoleMaxLines", "ConsoleOverflowStop" }) {
            settings->registerSetting(key, false);
        }
        return settings;
    }

    static std::unique_ptr<MinecraftInstance> makeInstance(SettingsObject* global, const QString& root)
    {
        QDir().mkpath(root);
        auto instance = std::make_unique<TestMinecraftInstance>(global, std::make_unique<INISettingsObject>(root + "/instance.cfg"), root);
        return instance;
    }

    static bool write(const QString& path, const QByteArray& value)
    {
        if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
            return false;
        }
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(value) == value.size();
    }

    static QByteArray read(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            return {};
        }
        return file.readAll();
    }
};

QTEST_GUILESS_MAIN(SharedContentManagerTest)

#include "SharedContentManager_test.moc"
