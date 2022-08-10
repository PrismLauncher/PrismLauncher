#pragma once

#include "ResourceFolderModel.h"

class ResourcePackFolderModel : public ResourceFolderModel
{
    Q_OBJECT
public:
    explicit ResourcePackFolderModel(const QString &dir);
};
