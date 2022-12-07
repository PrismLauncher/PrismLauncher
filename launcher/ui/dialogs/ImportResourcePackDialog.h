#pragma once

#include <QDialog>
#include <QItemSelection>

#include "ui/instanceview/InstanceGridProxyModel.h"

namespace Ui {
class ImportResourcePackDialog;
}

class ImportResourcePackDialog : public QDialog {
    Q_OBJECT

   public:
    explicit ImportResourcePackDialog(QWidget* parent = 0);
    ~ImportResourcePackDialog();
    InstanceGridProxyModel* proxyModel;
    QString selectedInstanceKey;

   private:
    Ui::ImportResourcePackDialog* ui;

   private slots:
    void selectionChanged(QItemSelection, QItemSelection);
    void activated(QModelIndex);
};
