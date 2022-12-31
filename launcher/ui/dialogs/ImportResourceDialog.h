#pragma once

#include <QDialog>
#include <QItemSelection>

#include "ui/instanceview/InstanceProxyModel.h"
#include "minecraft/mod/tasks/LocalResourceParse.h"

namespace Ui {
class ImportResourceDialog;
}

class ImportResourceDialog : public QDialog {
    Q_OBJECT

   public:
    explicit ImportResourceDialog(QString file_path, PackedResourceType type, QWidget* parent = 0);
    ~ImportResourceDialog();
    InstanceProxyModel* proxyModel;
    QString selectedInstanceKey;

   private:
    Ui::ImportResourceDialog* ui;
    PackedResourceType m_resource_type;
    QString m_file_path;

   private slots:
    void selectionChanged(QItemSelection, QItemSelection);
    void activated(QModelIndex);
};
