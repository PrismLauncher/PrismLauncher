#pragma once
#include "minecraft/MinecraftInstance.h"
#include <FileSystem.h>
#include "ui/pages/BasePage.h"
#include "ui/pages/BasePageProvider.h"
#include "ui/pages/instance/LogPage.h"
#include "ui/pages/instance/VersionPage.h"
#include "ui/pages/instance/ModFolderPage.h"
#include "ui/pages/instance/ResourcePackPage.h"
#include "ui/pages/instance/TexturePackPage.h"
#include "ui/pages/instance/ShaderPackPage.h"
#include "ui/pages/instance/NotesPage.h"
#include "ui/pages/instance/ScreenshotsPage.h"
#include "ui/pages/instance/InstanceSettingsPage.h"
#include "ui/pages/instance/OtherLogsPage.h"
#include "ui/pages/instance/WorldListPage.h"
#include "ui/pages/instance/ServersPage.h"
#include "ui/pages/instance/GameOptionsPage.h"

class InstancePageProvider : public QObject, public BasePageProvider
{
    Q_OBJECT
public:
    explicit InstancePageProvider(InstancePtr parent)
    {
        inst = parent;
    }

    virtual ~InstancePageProvider() {};
    virtual QList<BasePage *> getPages() override
    {
        QList<BasePage *> values;
        values.append(new LogPage(inst));
        std::shared_ptr<MinecraftInstance> onesix = std::dynamic_pointer_cast<MinecraftInstance>(inst);
        values.append(new VersionPage(onesix.get()));
        auto modsPage = new ModFolderPage(onesix.get(), onesix->loaderModList());
        modsPage->setFilter("%1 (*.zip *.jar *.litemod)");
        values.append(modsPage);
        values.append(new CoreModFolderPage(onesix.get(), onesix->coreModList()));
        values.append(new ResourcePackPage(onesix.get()));
        values.append(new TexturePackPage(onesix.get()));
        values.append(new ShaderPackPage(onesix.get()));
        values.append(new NotesPage(onesix.get()));
        values.append(new WorldListPage(onesix.get(), onesix->worldList()));
        values.append(new ServersPage(onesix));
        // values.append(new GameOptionsPage(onesix.get()));
        values.append(new ScreenshotsPage(FS::PathCombine(onesix->gameRoot(), "screenshots")));
        values.append(new InstanceSettingsPage(onesix.get()));
        auto logMatcher = inst->getLogFileMatcher();
        if(logMatcher)
        {
            values.append(new OtherLogsPage(inst->getLogFileRoot(), logMatcher));
        }
        return values;
    }

    virtual QString dialogTitle() override
    {
        return tr("Edit Instance (%1)").arg(inst->name());
    }
protected:
    InstancePtr inst;
};

