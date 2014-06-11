#pragma once
#include "ModFolderPage.h"

class ResourcePackPage : public ModFolderPage
{
public:
	explicit ResourcePackPage(BaseInstance *instance, QWidget *parent = 0)
		: ModFolderPage(instance->resourcePackList(), "resourcepacks", "resourcepacks",
						tr("Resource packs"), "Resource-packs", parent)
	{
		m_inst = instance;
	}

	virtual ~ResourcePackPage() {};
	virtual bool shouldDisplay() override
	{
		return !m_inst->traits().contains("no-texturepacks") &&
			   !m_inst->traits().contains("texturepacks");
	}
private:
	BaseInstance *m_inst;
};
