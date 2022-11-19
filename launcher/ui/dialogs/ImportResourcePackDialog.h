#pragma once

#include <QDialog>
#include <QItemSelection>

namespace Ui {
class ImportResourcePackDialog;
}

class ImportResourcePackDialog : public QDialog {
    Q_OBJECT

   public:
    explicit ImportResourcePackDialog(QWidget* parent = 0);
    ~ImportResourcePackDialog();
    QString selectedInstanceKey;

   private:
    Ui::ImportResourcePackDialog* ui;

   private slots:
    void selectionChanged(QItemSelection, QItemSelection);
    void activated(QModelIndex);
};
