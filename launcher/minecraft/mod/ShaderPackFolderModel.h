#pragma once

#include "ResourceFolderModel.h"

class ShaderPackFolderModel : public ResourceFolderModel {
    Q_OBJECT

   public:
    explicit ShaderPackFolderModel(const QString& dir, const BaseInstance* instance) : ResourceFolderModel(QDir(dir), instance) {}
};
