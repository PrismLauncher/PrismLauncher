#pragma once

#include <QDialog>
#include <QItemSelection>

#include "api/structures/ResourceType.h"
#include "ui/instanceview/InstanceProxyModel.h"

namespace Ui {
class ImportResourceDialog;
}

class ImportResourceDialog : public QDialog {
    Q_OBJECT

   public:
    explicit ImportResourceDialog(QString file_path, Platform::ResourceType type, QWidget* parent = nullptr);
    ~ImportResourceDialog() override;
    QString selectedInstanceKey;

   private:
    Ui::ImportResourceDialog* ui;
    Platform::ResourceType m_resource_type;
    QString m_file_path;
    InstanceProxyModel* proxyModel;

   private slots:
    void selectionChanged(QItemSelection, QItemSelection);
    void activated(QModelIndex);
};
