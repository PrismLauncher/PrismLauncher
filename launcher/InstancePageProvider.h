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
    explicit InstancePageProvider(MinecraftInstance* parent) { inst = parent; }

    virtual ~InstancePageProvider() = default;
    virtual QList<PageInfo> getPages() override
    {
        QList<PageInfo> values;
        values.append(LogPage::page(inst));
        values.append(VersionPage::page(inst));
        values.append(ManagedPackPage::page(inst));
        values.append(ModFolderPage::page(inst, inst->loaderModList()));
        values.append(ModFolderPage::coreModsPage(inst, inst->coreModList()));
        values.append(ModFolderPage::nilModsPage(inst, inst->nilModList()));
        values.append(ResourcePackPage::page(inst, inst->resourcePackList()));
        // values.append(GlobalDataPackPage::page(inst));
        values.append(TexturePackPage::page(inst, inst->texturePackList()));
        values.append(ShaderPackPage::page(inst, inst->shaderPackList()));
        values.append(NotesPage::page(inst));
        values.append(WorldListPage::page(inst, inst->worldList()));
        values.append(ServersPage::page(inst));
        values.append(ScreenshotsPage::page(FS::PathCombine(inst->gameRoot(), "screenshots")));
        values.append(InstanceSettingsPage::page(inst));
        values.append(OtherLogsPage::page(inst));
        return values;
    }

    virtual QString dialogTitle() override { return tr("Edit Instance (%1)").arg(inst->name()); }

   protected:
    MinecraftInstance* inst;
};
