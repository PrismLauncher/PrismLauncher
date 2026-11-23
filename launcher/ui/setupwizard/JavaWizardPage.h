#pragma once

#include "BaseWizardPage.h"

class JavaEnvironment;
class JavaWizardWidget;
namespace Meta {
class Index;
}

class JavaWizardPage : public BaseWizardPage {
    Q_OBJECT
   public:
    explicit JavaWizardPage(JavaEnvironment* java, Meta::Index* metadata, QWidget* parent = nullptr);

    virtual ~JavaWizardPage() = default;

    bool wantsRefreshButton() override;
    void refresh() override;
    void initializePage() override;
    bool validatePage() override;

   protected: /* methods */
    void setupUi();
    void retranslate() override;

   private: /* data */
    JavaWizardWidget* m_java_widget = nullptr;
    JavaEnvironment* m_java = nullptr;
    Meta::Index* m_metadata = nullptr;
};
