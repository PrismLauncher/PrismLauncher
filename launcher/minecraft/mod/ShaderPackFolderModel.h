#pragma once

#include "ResourceFolderModel.h"

class ShaderPackFolderModel : public ResourceFolderModel {
    Q_OBJECT

   public:
    explicit ShaderPackFolderModel(const QDir& dir, BaseInstance* instance, bool is_indexed, bool create_dir, QObject* parent = nullptr)
        : ResourceFolderModel(dir, instance, is_indexed, create_dir, parent)
    {}

    virtual QString id() const override { return "shaderpacks"; }
};
