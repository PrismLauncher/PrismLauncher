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
    auto s = APPLICATION->settings();
    QString prevPasteURL = s->get("PastebinURL").toString();
    s->reset("PastebinURL");
    if (ui->previousSettingsRadioButton->isChecked()) {
        bool usingDefaultBase =
            prevPasteURL == PasteUpload::g_PasteTypes.at(static_cast<std::size_t>(PasteUpload::PasteType::NullPointer)).defaultBase;
        s->set("PastebinType", static_cast<int>(PasteUpload::PasteType::NullPointer));
        if (!usingDefaultBase)
            s->set("PastebinCustomAPIBase", prevPasteURL);
    }

    return true;
}

void PasteWizardPage::retranslate()
{
    ui->retranslateUi(this);
}
