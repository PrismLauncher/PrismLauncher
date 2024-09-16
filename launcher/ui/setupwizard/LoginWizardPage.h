#pragma once
#include <QWidget>
#include "BaseWizardPage.h"

namespace Ui {
class LoginWizardPage;
}

class LoginWizardPage : public BaseWizardPage {
    Q_OBJECT

   public:
    explicit LoginWizardPage(QWidget* parent = nullptr);
    ~LoginWizardPage();

    void initializePage() override;
    bool validatePage() override;
    void retranslate() override;
   private slots:
    void on_pushButton_clicked();

   private:
    Ui::LoginWizardPage* ui;
};
