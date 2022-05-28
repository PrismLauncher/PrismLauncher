#pragma once

#include <QDialog>


QT_BEGIN_NAMESPACE
namespace Ui { class ScrollMessageBox; }
QT_END_NAMESPACE

class ScrollMessageBox : public QDialog {
Q_OBJECT

public:
    ScrollMessageBox(QWidget *parent, const QString &title, const QString &text, const QString &body);

    ~ScrollMessageBox() override;

private:
    Ui::ScrollMessageBox *ui;
};
