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
    void on_loginButton_clicked();
    void on_skipButton_clicked();
    void on_accountTypeCombo_currentIndexChanged(int index);

   private:
    enum class DesiredAccountType { Offline, ElyBy, Microsoft };

    DesiredAccountType selectedType() const;
    bool addSelectedAccount();
    void updateUiForSelection();

   private:
    Ui::LoginWizardPage* ui;
    bool m_accountAdded = false;
    bool m_skipped = false;
};
