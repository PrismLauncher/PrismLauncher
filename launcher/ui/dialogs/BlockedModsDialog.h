#pragma once

#include <QDialog>


struct BlockedMod {
    QString name;
    QString websiteUrl;
    QString hash;
    bool matched;
    QString localPath;

};

QT_BEGIN_NAMESPACE
namespace Ui { class BlockedModsDialog; }
QT_END_NAMESPACE

class BlockedModsDialog : public QDialog {
Q_OBJECT

public:
    BlockedModsDialog(QWidget *parent, const QString &title, const QString &text, const QList<BlockedMod> &mods);

    ~BlockedModsDialog() override;


private:
    Ui::BlockedModsDialog *ui;
    const QList<BlockedMod> &mods;
    void openAll();
    void update();
};

