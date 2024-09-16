#pragma once
#include <QWidget>
#include "BaseWizardPage.h"

namespace Ui {
class AutoJavaWizardPage;
}

class AutoJavaWizardPage : public BaseWizardPage {
    Q_OBJECT

   public:
    explicit AutoJavaWizardPage(QWidget* parent = nullptr);
    ~AutoJavaWizardPage();

    void initializePage() override;
    bool validatePage() override;
    void retranslate() override;

   private:
    Ui::AutoJavaWizardPage* ui;
};
