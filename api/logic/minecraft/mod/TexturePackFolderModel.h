#pragma once

#include "ModFolderModel.h"

class MULTIMC_LOGIC_EXPORT TexturePackFolderModel : public ModFolderModel
{
    Q_OBJECT

public:
    explicit TexturePackFolderModel(const QString &dir);

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
};
