#include "AutoJavaWizardPage.h"
#include "ui_AutoJavaWizardPage.h"

#include "Application.h"

AutoJavaWizardPage::AutoJavaWizardPage(QWidget* parent) : BaseWizardPage(parent), ui(new Ui::AutoJavaWizardPage)
{
    ui->setupUi(this);
}

AutoJavaWizardPage::~AutoJavaWizardPage()
{
    delete ui;
}

void AutoJavaWizardPage::initializePage() {}

bool AutoJavaWizardPage::validatePage()
{
    auto s = APPLICATION->settings();

    if (!ui->previousSettingsRadioButton->isChecked()) {
        s->set("AutomaticJavaSwitch", true);
        s->set("AutomaticJavaDownload", true);
    }
    s->set("UserAskedAboutAutomaticJavaDownload", true);
    return true;
}

void AutoJavaWizardPage::retranslate()
{
    ui->retranslateUi(this);
}
