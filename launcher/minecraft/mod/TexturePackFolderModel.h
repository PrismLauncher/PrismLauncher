#pragma once

#include "ResourceFolderModel.h"

class TexturePackFolderModel : public ResourceFolderModel
{
    Q_OBJECT

public:
    explicit TexturePackFolderModel(const QString &dir);
};
