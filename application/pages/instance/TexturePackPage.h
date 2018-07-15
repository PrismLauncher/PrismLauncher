#pragma once
#include "ModFolderPage.h"
#include "ui_ModFolderPage.h"

class TexturePackPage : public ModFolderPage
{
public:
    explicit TexturePackPage(MinecraftInstance *instance, QWidget *parent = 0)
        : ModFolderPage(instance, instance->texturePackList(), "texturepacks", "resourcepacks",
                        tr("Texture packs"), "Texture-packs", parent)
    {
        ui->configFolderBtn->setHidden(true);
    }
    virtual ~TexturePackPage() {}
    virtual bool shouldDisplay() const override
    {
        return m_inst->traits().contains("texturepacks");
    }
};
