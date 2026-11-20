#include <QTest>

#include "modplatform/ModPlatformRegistry.h"

class ModPlatformRegistryTest : public QObject {
    Q_OBJECT

   private slots:
    void init() { ModPlatform::ModPlatformRegistry::instance().resetForTesting(); }
    void cleanup() { ModPlatform::ModPlatformRegistry::instance().resetForTesting(); }

    void registerAndFind()
    {
        using namespace ModPlatform;

        ModPlatformProvider provider;
        provider.id = QStringLiteral("test-provider");
        provider.displayName = QStringLiteral("Test Provider");
        provider.iconName = QStringLiteral("test");
        provider.cacheKey = QStringLiteral("TestPacks");
        provider.capabilities = ProviderCapability::NewInstancePage | ProviderCapability::HasResourceAPI;

        auto& registry = ModPlatformRegistry::instance();
        QVERIFY(registry.registerProvider(std::move(provider)));

        const auto* found = registry.find(QStringLiteral("test-provider"));
        QVERIFY(found != nullptr);
        QCOMPARE(found->displayName, QStringLiteral("Test Provider"));
        QCOMPARE(found->cacheKey, QStringLiteral("TestPacks"));
        QVERIFY(found->hasCapability(ProviderCapability::NewInstancePage));
        QVERIFY(found->hasCapability(ProviderCapability::HasResourceAPI));
        QVERIFY(!found->hasCapability(ProviderCapability::RequiresApiKey));
        QVERIFY(found->available());
    }

    void rejectEmptyAndDuplicateIds()
    {
        using namespace ModPlatform;
        auto& registry = ModPlatformRegistry::instance();

        ModPlatformProvider empty;
        QVERIFY(!registry.registerProvider(empty));

        ModPlatformProvider provider;
        provider.id = QStringLiteral("dup");
        QVERIFY(registry.registerProvider(provider));
        QVERIFY(!registry.registerProvider(provider));
        QCOMPARE(registry.providers().size(), 1);
    }

    void filterByCapabilityAndAvailability()
    {
        using namespace ModPlatform;
        auto& registry = ModPlatformRegistry::instance();

        ModPlatformProvider modsOnly;
        modsOnly.id = QStringLiteral("mods-only");
        modsOnly.capabilities = ProviderCapability::ResourceMods;
        QVERIFY(registry.registerProvider(modsOnly));

        ModPlatformProvider flameLike;
        flameLike.id = QStringLiteral("flame-like");
        flameLike.capabilities = ProviderCapability::ResourceMods | ProviderCapability::RequiresApiKey;
        flameLike.isAvailable = [] { return false; };
        QVERIFY(registry.registerProvider(flameLike));

        ModPlatformProvider packs;
        packs.id = QStringLiteral("packs");
        packs.capabilities = ProviderCapability::NewInstancePage | ProviderCapability::InstallTask;
        QVERIFY(registry.registerProvider(packs));

        const auto modProviders = registry.providersWith(ProviderCapability::ResourceMods);
        QCOMPARE(modProviders.size(), 1);
        QCOMPARE(modProviders.first()->id, QStringLiteral("mods-only"));

        const auto packProviders = registry.providersWith(ProviderCapability::NewInstancePage);
        QCOMPARE(packProviders.size(), 1);
        QCOMPARE(packProviders.first()->id, QStringLiteral("packs"));

        QCOMPARE(registry.providers().size(), 3);
    }

    void resourceTypeCapabilityMapping()
    {
        using namespace ModPlatform;
        QCOMPARE(capabilityForResourceType(ResourceType::Mod), ProviderCapability::ResourceMods);
        QCOMPARE(capabilityForResourceType(ResourceType::ResourcePack), ProviderCapability::ResourcePacks);
        QCOMPARE(capabilityForResourceType(ResourceType::TexturePack), ProviderCapability::TexturePacks);
        QCOMPARE(capabilityForResourceType(ResourceType::ShaderPack), ProviderCapability::ShaderPacks);
        QCOMPARE(capabilityForResourceType(ResourceType::DataPack), ProviderCapability::DataPacks);
        QCOMPARE(capabilityForResourceType(ResourceType::Modpack), ProviderCapability::NewInstancePage);
        QCOMPARE(capabilityForResourceType(ResourceType::Unknown), ProviderCapability::None);
    }

    void resourcePageFactoryLookup()
    {
        using namespace ModPlatform;

        ModPlatformProvider provider;
        provider.id = QStringLiteral("factory-test");
        provider.capabilities = ProviderCapability::ResourceMods;
        provider.createModPage = [](ResourceDownload::ResourceDownloadDialog*, BaseInstance&) -> BasePage* { return nullptr; };

        QVERIFY(static_cast<bool>(provider.resourcePageFactory(ResourceType::Mod)));
        QVERIFY(!static_cast<bool>(provider.resourcePageFactory(ResourceType::ShaderPack)));
        QVERIFY(!static_cast<bool>(provider.resourcePageFactory(ResourceType::World)));
    }
};

QTEST_GUILESS_MAIN(ModPlatformRegistryTest)

#include "ModPlatformRegistry_test.moc"
