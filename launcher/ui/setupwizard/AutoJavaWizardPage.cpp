#include "AutoJavaWizardPage.h"
#include "config/GlobalConfig.h"
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
    auto& conf = APPLICATION->config().update();

    if (!ui->previousSettingsRadioButton->isChecked()) {
        conf.automaticJavaSwitch = true;
        conf.automaticJavaDownload = true;
    }
    conf.userAskedAboutAutomaticJavaDownload = true;
    return true;
}

void AutoJavaWizardPage::retranslate()
{
    ui->retranslateUi(this);
}
