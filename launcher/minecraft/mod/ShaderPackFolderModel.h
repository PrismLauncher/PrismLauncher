#pragma once

#include "ResourceFolderModel.h"

class ShaderPackFolderModel : public ResourceFolderModel {
    Q_OBJECT

   public:
    explicit ShaderPackFolderModel(const QString& dir, std::shared_ptr<BaseInstance> instance)
        : ResourceFolderModel(QDir(dir), instance)
    {}
};
