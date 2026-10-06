#pragma once

#include <QDialog>
#include <memory>

namespace Ui {
class OfflineModeGreetingDialog;
}

class Application;

class OfflineModeGreetingDialog : public QDialog {
    Q_OBJECT

public:
    explicit OfflineModeGreetingDialog(QWidget* parent = nullptr);
    ~OfflineModeGreetingDialog();

private slots:
    void on_addMicrosoftButton_clicked();
    void on_addOfflineButton_clicked();
    void on_skipButton_clicked();

private:
    Ui::OfflineModeGreetingDialog* ui;
    Application* m_app;
};
