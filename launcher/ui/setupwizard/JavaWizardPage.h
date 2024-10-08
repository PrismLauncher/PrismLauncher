#pragma once

#include "BaseWizardPage.h"

class JavaSettingsWidget;

class JavaWizardPage : public BaseWizardPage {
    Q_OBJECT
   public:
    explicit JavaWizardPage(QWidget* parent = Q_NULLPTR);

    virtual ~JavaWizardPage() = default;

    bool wantsRefreshButton() override;
    void refresh() override;
    void initializePage() override;
    bool validatePage() override;

   protected: /* methods */
    void setupUi();
    void retranslate() override;

   private: /* data */
    JavaSettingsWidget* m_java_widget = nullptr;
};
