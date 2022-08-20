#pragma once

#include <QDialog>


QT_BEGIN_NAMESPACE
namespace Ui { class BlockedModsDialog; }
QT_END_NAMESPACE

class BlockedModsDialog : public QDialog {
Q_OBJECT

public:
    BlockedModsDialog(QWidget *parent, const QString &title, const QString &text, const QString &body, const QList<QUrl> &urls);

    ~BlockedModsDialog() override;

private:
    Ui::BlockedModsDialog *ui;
    const QList<QUrl> &urls;
    void openAll();
};
