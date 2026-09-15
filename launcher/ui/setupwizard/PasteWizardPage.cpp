#include "PasteWizardPage.h"
#include "ui_PasteWizardPage.h"

#include "Application.h"
#include "net/PasteUpload.h"
#include "settings/SettingsObject.h"

PasteWizardPage::PasteWizardPage(QWidget* parent) : BaseWizardPage(parent), ui(new Ui::PasteWizardPage)
{
    ui->setupUi(this);
}

PasteWizardPage::~PasteWizardPage()
{
    delete ui;
}

void PasteWizardPage::initializePage() {}

bool PasteWizardPage::validatePage()
{
    auto* s = APPLICATION->settings();
    QString prevPasteURL = s->get("PastebinURL").toString();
    s->reset("PastebinURL");
    if (ui->previousSettingsRadioButton->isChecked()) {
        auto nullPointer = PasteUpload::Type(PasteUpload::Type::NullPointer);
        bool usingDefaultBase = prevPasteURL == nullPointer.defaultBase();
        s->set("PastebinType", nullPointer.toInt());
        if (!usingDefaultBase) {
            s->set("PastebinCustomAPIBase", prevPasteURL);
        }
    }

    return true;
}

void PasteWizardPage::retranslate()
{
    ui->retranslateUi(this);
}
