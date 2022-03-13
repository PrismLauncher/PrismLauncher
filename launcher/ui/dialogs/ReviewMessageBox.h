#pragma once

#include <QDialog>

namespace Ui {
class ReviewMessageBox;
}

class ReviewMessageBox final : public QDialog {
    Q_OBJECT

   public:
    static auto create(QWidget* parent, QString&& title, QString&& icon = "") -> ReviewMessageBox*;

    void appendMod(const QString& name, const QString& filename);

    ~ReviewMessageBox();

   private:
    ReviewMessageBox(QWidget* parent, const QString& title, const QString& icon);

    Ui::ReviewMessageBox* ui;
};
