#pragma once

#include "ResourceFolderModel.h"
#include "minecraft/mod/ShaderPack.h"
#include "minecraft/mod/tasks/BasicFolderLoadTask.h"
#include "minecraft/mod/tasks/LocalShaderPackParseTask.h"

class ShaderPackFolderModel : public ResourceFolderModel {
    Q_OBJECT

   public:
    explicit ShaderPackFolderModel(const QString& dir, BaseInstance* instance) : ResourceFolderModel(QDir(dir), instance) {}

    virtual QString id() const override { return "shaderpacks"; }

    [[nodiscard]] TaskV2* createUpdateTask() override
    {
        return new BasicFolderLoadTask(m_dir, [](QFileInfo const& entry) { return makeShared<ShaderPack>(entry); });
    }

    [[nodiscard]] TaskV2* createParseTask(Resource& resource) override
    {
        return new LocalShaderPackParseTask(m_next_resolution_ticket, static_cast<ShaderPack&>(resource));
    }
};
