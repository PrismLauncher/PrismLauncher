#pragma once
#include "ModFolderPage.h"

class TexturePackPage : public ModFolderPage
{
public:
	explicit TexturePackPage(BaseInstance *instance, QWidget *parent = 0)
		: ModFolderPage(instance, instance->texturePackList(), "texturepacks", "resourcepacks",
						tr("Texture packs"), "Texture-packs", parent)
	{
	}
	virtual ~TexturePackPage() {}
	virtual bool shouldDisplay() const override
	{
		return m_inst->traits().contains("texturepacks");
	}
};
