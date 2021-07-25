#include "TexturePackFolderModel.h"

TexturePackFolderModel::TexturePackFolderModel(const QString &dir) : ModFolderModel(dir) {
}

QVariant TexturePackFolderModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::ToolTipRole) {
        switch (section) {
            case ActiveColumn:
                return tr("Is the texture pack enabled?");
            case NameColumn:
                return tr("The name of the texture pack.");
            case VersionColumn:
                return tr("The version of the texture pack.");
            case DateColumn:
                return tr("The date and time this texture pack was last changed (or added).");
            default:
                return QVariant();
        }
    }

    return ModFolderModel::headerData(section, orientation, role);
}
