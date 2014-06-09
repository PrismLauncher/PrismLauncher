#pragma once
#include "ModFolderPage.h"

class TexturePackPage : public ModFolderPage
{
public:
	explicit TexturePackPage(BaseInstance *instance, QWidget *parent = 0)
		: ModFolderPage(instance->texturePackList(), "texturepacks", "resourcepacks",
						tr("Texture packs"), "ResourcePacksPage", parent)
	{
		m_inst = instance;
	}
	virtual ~TexturePackPage() {};
	virtual bool shouldDisplay() override
	{
		return m_inst->traits().contains("texturepacks");
	}
private:
	BaseInstance *m_inst;
};
