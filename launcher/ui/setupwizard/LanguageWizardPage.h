#pragma once

#include "BaseWizardPage.h"

class LanguageSelectionWidget;

class LanguageWizardPage : public BaseWizardPage
{
    Q_OBJECT
public:
    explicit LanguageWizardPage(QWidget *parent = Q_NULLPTR);

    virtual ~LanguageWizardPage();

    bool wantsRefreshButton() override;

    void refresh() override;

    bool validatePage() override;

protected:
    void retranslate() override;

private:
    LanguageSelectionWidget *mainWidget = nullptr;
};
