#pragma once

#include "ResourceFolderModel.h"

#include "ResourcePack.h"

class ResourcePackFolderModel : public ResourceFolderModel
{
    Q_OBJECT
public:
    explicit ResourcePackFolderModel(const QString &dir);

    RESOURCE_HELPERS(ResourcePack)
};
