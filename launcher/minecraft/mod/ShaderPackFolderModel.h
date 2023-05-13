#pragma once

#include "ResourceFolderModel.h"

class ShaderPackFolderModel : public ResourceFolderModel {
    Q_OBJECT

   public:
    explicit ShaderPackFolderModel(const QString& dir, std::shared_ptr<const BaseInstance> instance)
        : ResourceFolderModel(QDir(dir), instance)
    {}
};
