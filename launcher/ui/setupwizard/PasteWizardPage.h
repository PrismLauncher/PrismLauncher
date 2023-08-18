#ifndef PASTEDEFAULTSCONFIRMATIONWIZARD_H
#define PASTEDEFAULTSCONFIRMATIONWIZARD_H

#include <QWidget>
#include "BaseWizardPage.h"

namespace Ui {
class PasteWizardPage;
}

class PasteWizardPage : public BaseWizardPage {
    Q_OBJECT

   public:
    explicit PasteWizardPage(QWidget* parent = nullptr);
    ~PasteWizardPage();

    void initializePage() override;
    bool validatePage() override;
    void retranslate() override;

   private:
    Ui::PasteWizardPage* ui;
};

#endif  // PASTEDEFAULTSCONFIRMATIONWIZARD_H
