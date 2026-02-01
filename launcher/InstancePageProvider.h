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
    explicit InstancePageProvider(BaseInstance* parent) { inst = parent; }

    virtual ~InstancePageProvider() = default;
    virtual QList<BasePage*> getPages() override
    {
        QList<BasePage*> values;
        values.append(new LogPage(inst));
        MinecraftInstance* onesix = dynamic_cast<MinecraftInstance*>(inst);
        values.append(new VersionPage(onesix));
        values.append(ManagedPackPage::createPage(onesix));
        auto modsPage = new ModFolderPage(onesix, onesix->loaderModList());
        modsPage->setFilter("%1 (*.zip *.jar *.litemod *.nilmod)");
        values.append(modsPage);
        values.append(new CoreModFolderPage(onesix, onesix->coreModList()));
        values.append(new NilModFolderPage(onesix, onesix->nilModList()));
        values.append(new ResourcePackPage(onesix, onesix->resourcePackList()));
        values.append(new GlobalDataPackPage(onesix));
        values.append(new TexturePackPage(onesix, onesix->texturePackList()));
        values.append(new ShaderPackPage(onesix, onesix->shaderPackList()));
        values.append(new NotesPage(onesix));
        values.append(new WorldListPage(onesix, onesix->worldList()));
        values.append(new ServersPage(onesix));
        values.append(new ScreenshotsPage(FS::PathCombine(onesix->gameRoot(), "screenshots")));
        values.append(new InstanceSettingsPage(onesix));
        values.append(new OtherLogsPage("logs", tr("Other Logs"), "Other-Logs", inst));
        return values;
    }

    virtual QString dialogTitle() override { return tr("Edit Instance (%1)").arg(inst->name()); }

   protected:
    BaseInstance* inst;
};
