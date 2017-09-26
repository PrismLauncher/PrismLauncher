#pragma once
#include "minecraft/MinecraftInstance.h"
#include "minecraft/legacy/LegacyInstance.h"
#include <FileSystem.h>
#include "pages/BasePage.h"
#include "pages/LogPage.h"
#include "pages/VersionPage.h"
#include "pages/ModFolderPage.h"
#include "pages/ResourcePackPage.h"
#include "pages/TexturePackPage.h"
#include "pages/NotesPage.h"
#include "pages/ScreenshotsPage.h"
#include "pages/InstanceSettingsPage.h"
#include "pages/OtherLogsPage.h"
#include "pages/BasePageProvider.h"
#include "pages/LegacyUpgradePage.h"
#include "pages/WorldListPage.h"


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
		if(onesix)
		{
			values.append(new VersionPage(onesix.get()));
			auto modsPage = new ModFolderPage(onesix.get(), onesix->loaderModList(), "mods", "loadermods", tr("Loader mods"), "Loader-mods");
			modsPage->setFilter("%1 (*.zip *.jar *.litemod)");
			values.append(modsPage);
			values.append(new CoreModFolderPage(onesix.get(), onesix->coreModList(), "coremods", "coremods", tr("Core mods"), "Core-mods"));
			values.append(new ResourcePackPage(onesix.get()));
			values.append(new TexturePackPage(onesix.get()));
			values.append(new NotesPage(onesix.get()));
			values.append(new WorldListPage(onesix.get(), onesix->worldList(), "worlds", "worlds", tr("Worlds"), "Worlds"));
			values.append(new ScreenshotsPage(FS::PathCombine(onesix->minecraftRoot(), "screenshots")));
			values.append(new InstanceSettingsPage(onesix.get()));
		}
		std::shared_ptr<LegacyInstance> legacy = std::dynamic_pointer_cast<LegacyInstance>(inst);
		if(legacy)
		{
			values.append(new LegacyUpgradePage(legacy));
			values.append(new NotesPage(legacy.get()));
			values.append(new WorldListPage(legacy.get(), legacy->worldList(), "worlds", "worlds", tr("Worlds"), "Worlds"));
			values.append(new ScreenshotsPage(FS::PathCombine(legacy->minecraftRoot(), "screenshots")));
		}
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
