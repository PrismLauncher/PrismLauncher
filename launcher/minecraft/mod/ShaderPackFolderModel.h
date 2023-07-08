#pragma once

#include "ResourceFolderModel.h"

class ShaderPackFolderModel : public ResourceFolderModel {
    Q_OBJECT

   public:
    explicit ShaderPackFolderModel(const QString& dir, BaseInstance* instance)
        : ResourceFolderModel(QDir(dir), instance)
    {}
    
    virtual QString id() const override { return "shaderpacks"; }
};
