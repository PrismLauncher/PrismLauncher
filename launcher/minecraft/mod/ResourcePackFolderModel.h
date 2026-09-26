#pragma once

#include "ResourceFolderModel.h"

#include "ResourcePack.h"

class ResourcePackFolderModel : public ResourceFolderModel {
    Q_OBJECT
   public:
    enum Columns : std::uint8_t {
        ActiveColumn = 0,
        NameColumn,
        VersionColumn,
        PackFormatColumn,
        DateColumn,
        ProviderColumn,
        SizeColumn,
        FileNameColumn,
        LockUpdateColumn,
        NumColumns
    };

    explicit ResourcePackFolderModel(const QDir& dir,
                                     MinecraftInstance* instance,
                                     bool isIndexed,
                                     bool createDir,
                                     QObject* parent = nullptr);

    QString id() const override { return "resourcepacks"; }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    QList<MultiDecorationItemDelegate::Icon> icons(int row) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int columnCount(const QModelIndex& parent) const override;

    [[nodiscard]] Resource* createResource(const QFileInfo& file) override { return new ResourcePack(file); }
    [[nodiscard]] Task* createParseTask(Resource& /*unused*/) override;

    bool showImageToggle() override { return true; }

    RESOURCE_HELPERS(ResourcePack)
};
