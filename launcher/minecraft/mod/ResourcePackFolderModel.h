#pragma once

#include "ResourceFolderModel.h"

#include "ResourcePack.h"

class ResourcePackFolderModel : public ResourceFolderModel
{
    Q_OBJECT
public:
    enum Columns
    {
        ActiveColumn = 0,
        NameColumn,
        PackFormatColumn,
        DateColumn,
        NUM_COLUMNS
    };

    explicit ResourcePackFolderModel(const QString &dir, std::shared_ptr<const BaseInstance> instance);

    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent) const override;

    [[nodiscard]] Task* createUpdateTask() override;
    [[nodiscard]] Task* createParseTask(Resource&) override;

    RESOURCE_HELPERS(ResourcePack)
};
