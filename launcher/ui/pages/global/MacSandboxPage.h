//
// Created by Kenneth Chew on 11/11/25.
//

#ifndef LAUNCHER_MACSANDBOXPAGE_H
#define LAUNCHER_MACSANDBOXPAGE_H

#include <QMainWindow>
#include "ui/pages/BasePage.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MacSandboxPage;
}
QT_END_NAMESPACE

class MacSandboxPage : public QMainWindow, public BasePage {
    Q_OBJECT

public:
    explicit MacSandboxPage(QWidget* parent = nullptr);
    ~MacSandboxPage() override;
    void loadSettings();

    QString displayName() const override { return tr("Sandbox"); }
    QIcon icon() const override { return QIcon::fromTheme("settings"); }
    QString id() const override { return "launcher-mac-sandbox"; }
    QString helpPage() const override { return "Launcher-mac-sandbox"; }

private:
    Ui::MacSandboxPage* ui;

private slots:
    void on_readWriteAddBtn_clicked();
    void on_readWriteRemoveBtn_clicked();
    void on_readOnlyAddBtn_clicked();
    void on_readOnlyRemoveBtn_clicked();
};

#endif  // LAUNCHER_MACSANDBOXPAGE_H
