#pragma once
#include <FileSystem.h>
#include <ui/pages/instance/DataPackPage.h>
#include "minecraft/MinecraftInstance.h"
#include "ui/pages/BasePage.h"
#include "ui/pages/BasePageProvider.h"
#include "ui/pages/instance/InstanceSettingsPage.h"
#include "ui/pages/instance/LogPage.h"
#include "ui/pages/instance/ManagedPackPage.h"
#include "ui/pages/instance/ModFolderPage.h"
#include "ui/pages/instance/NotesPage.h"
#include "ui/pages/instance/OtherLogsPage.h"
#include "ui/pages/instance/OverviewPage.h"
#include "ui/pages/instance/ResourcePackPage.h"
#include "ui/pages/instance/ScreenshotsPage.h"
#include "ui/pages/instance/ServersPage.h"
#include "ui/pages/instance/ShaderPackPage.h"
#include "ui/pages/instance/TexturePackPage.h"
#include "ui/pages/instance/VersionPage.h"
#include "ui/pages/instance/WorldListPage.h"

class InstancePageProvider : protected QObject, public BasePageProvider {
    Q_OBJECT
   public:
    explicit InstancePageProvider(MinecraftInstance* parent) { inst = parent; }

    virtual ~InstancePageProvider() = default;
    virtual QList<BasePage*> getPages() override
    {
        QList<BasePage*> values;

        // Overview: what the instance is, how it plays and where its content lives
        values.append(new OverviewPage(inst));

        // Content: everything inside the instance folder that people add, remove and browse
        const QString contentSection = tr("Content");
        auto modsPage = new ModFolderPage(inst, inst->loaderModList());
        modsPage->setFilter("%1 (*.zip *.jar *.litemod *.nilmod)");
        modsPage->setSection(contentSection);
        values.append(modsPage);
        auto coreModsPage = new CoreModFolderPage(inst, inst->coreModList());
        coreModsPage->setSection(contentSection);
        values.append(coreModsPage);
        auto nilModsPage = new NilModFolderPage(inst, inst->nilModList());
        nilModsPage->setSection(contentSection);
        values.append(nilModsPage);
        auto resourcePacksPage = new ResourcePackPage(inst, inst->resourcePackList());
        resourcePacksPage->setSection(contentSection);
        values.append(resourcePacksPage);
        auto dataPacksPage = new GlobalDataPackPage(inst);
        dataPacksPage->setSection(contentSection);
        values.append(dataPacksPage);
        auto texturePacksPage = new TexturePackPage(inst, inst->texturePackList());
        texturePacksPage->setSection(contentSection);
        values.append(texturePacksPage);
        auto shaderPacksPage = new ShaderPackPage(inst, inst->shaderPackList());
        shaderPacksPage->setSection(contentSection);
        values.append(shaderPacksPage);
        auto notesPage = new NotesPage(inst);
        notesPage->setSection(contentSection);
        values.append(notesPage);
        auto worldsPage = new WorldListPage(inst, inst->worldList());
        worldsPage->setSection(contentSection);
        values.append(worldsPage);
        auto serversPage = new ServersPage(inst);
        serversPage->setSection(contentSection);
        values.append(serversPage);
        auto screenshotsPage = new ScreenshotsPage(FS::PathCombine(inst->gameRoot(), "screenshots"));
        screenshotsPage->setSection(contentSection);
        values.append(screenshotsPage);

        // Setup: what the instance is and how it starts
        const QString setupSection = tr("Setup");
        auto versionPage = new VersionPage(inst);
        versionPage->setSection(setupSection);
        values.append(versionPage);
        auto managedPackPage = ManagedPackPage::createPage(inst);
        managedPackPage->setSection(setupSection);
        values.append(managedPackPage);
        auto settingsPage = new InstanceSettingsPage(inst);
        settingsPage->setSection(setupSection);
        values.append(settingsPage);

        // Logs: the console and the other logs; the console still opens automatically on launch
        const QString logsSection = tr("Logs");
        auto logPage = new LogPage(inst);
        logPage->setSection(logsSection);
        values.append(logPage);
        auto otherLogsPage = new OtherLogsPage("logs", tr("Other Logs"), "Other-Logs", inst);
        otherLogsPage->setSection(logsSection);
        values.append(otherLogsPage);

        return values;
    }

    virtual QString dialogTitle() override { return tr("Edit Instance (%1)").arg(inst->name()); }

   protected:
    MinecraftInstance* inst;
};
