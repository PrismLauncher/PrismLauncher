#pragma once

#include "ResourceFolderModel.h"

#include "ResourcePack.h"

class ResourcePackFolderModel : public ResourceFolderModel {
    Q_OBJECT
   public:
    enum Columns : std::uint8_t {
        ActiveColumn = 0,
        ImageColumn,
        NameColumn,
        PackFormatColumn,
        DateColumn,
        ProviderColumn,
        SizeColumn,
        FileNameColumn,
        NumColumns
    };

    explicit ResourcePackFolderModel(const QDir& dir, BaseInstance* instance, bool isIndexed, bool createDir, QObject* parent = nullptr);

    QString id() const override { return "resourcepacks"; }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int columnCount(const QModelIndex& parent) const override;

    [[nodiscard]] Resource* createResource(const QFileInfo& file) override { return new ResourcePack(file); }
    [[nodiscard]] Task* createParseTask(Resource& /*unused*/) override;

    RESOURCE_HELPERS(ResourcePack)
};
