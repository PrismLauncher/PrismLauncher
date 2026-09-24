#pragma once

#include <QDialog>
#include <QItemSelection>

#include "modplatform/ResourceType.h"
#include "ui/instanceview/InstanceProxyModel.h"

namespace Ui {
class ImportResourceDialog;
}

class ImportResourceDialog : public QDialog {
    Q_OBJECT

   public:
    explicit ImportResourceDialog(QString filePath, ModPlatform::ResourceType type, QWidget* parent = nullptr);
    ~ImportResourceDialog() override;
    QString selectedInstanceKey;

    void sortBy(QStringList mcVersions, ModPlatform::ModLoaderTypes loader = ModPlatform::ModLoaderType::None);

   private:
    Ui::ImportResourceDialog* m_ui;
    ModPlatform::ResourceType m_resourceType;
    QString m_filePath;
    InstanceProxyModel* m_proxyModel;

    QStringList m_mcVersions;
    ModPlatform::ModLoaderTypes m_loader;

   private slots:
    void selectionChanged(QItemSelection, QItemSelection);
    void activated(QModelIndex);
    void showAllInstances(bool checked);
};
