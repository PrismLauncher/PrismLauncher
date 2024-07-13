#pragma once

#include "ResourceFolderModel.h"

#include "ResourcePack.h"

class ResourcePackFolderModel : public ResourceFolderModel {
    Q_OBJECT
   public:
    enum Columns { ActiveColumn = 0, ImageColumn, NameColumn, PackFormatColumn, DateColumn, SizeColumn, NUM_COLUMNS };

    explicit ResourcePackFolderModel(const QString& dir, BaseInstance* instance);

    virtual QString id() const override { return "resourcepacks"; }

    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent) const override;

    [[nodiscard]] TaskV2* createUpdateTask() override;
    [[nodiscard]] TaskV2* createParseTask(Resource&) override;

    RESOURCE_HELPERS(ResourcePack)
};
