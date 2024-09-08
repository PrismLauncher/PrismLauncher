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

    [[nodiscard]] Task* createUpdateTask() override
    {
        return new BasicFolderLoadTask(m_dir, [](QFileInfo const& entry) { return makeShared<ShaderPack>(entry); });
    }

    [[nodiscard]] Task* createParseTask(Resource& resource) override
    {
        return new LocalShaderPackParseTask(m_next_resolution_ticket, static_cast<ShaderPack&>(resource));
    }
};
