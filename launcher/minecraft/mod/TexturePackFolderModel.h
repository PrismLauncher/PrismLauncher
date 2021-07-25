#pragma once

#include "ModFolderModel.h"

class TexturePackFolderModel : public ModFolderModel
{
    Q_OBJECT

public:
    explicit TexturePackFolderModel(const QString &dir);

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
};
