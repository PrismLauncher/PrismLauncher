#pragma once

#include "ResourceFolderModel.h"
#include "minecraft/mod/ShaderPack.h"
#include "minecraft/mod/tasks/LocalShaderPackParseTask.h"

class ShaderPackFolderModel : public ResourceFolderModel {
    Q_OBJECT

   public:
    explicit ShaderPackFolderModel(const QDir& dir, BaseInstance* instance, bool is_indexed, bool create_dir, QObject* parent = nullptr)
        : ResourceFolderModel(dir, instance, is_indexed, create_dir, parent)
    {}

    virtual QString id() const override { return "shaderpacks"; }

    [[nodiscard]] Resource* createResource(const QFileInfo& info) override { return new ShaderPack(info); }

    [[nodiscard]] Task* createParseTask(Resource& resource) override
    {
        return new LocalShaderPackParseTask(m_next_resolution_ticket, static_cast<ShaderPack&>(resource));
    }

    RESOURCE_HELPERS(ShaderPack);
};
