#include "LoginWizardPage.h"

#include "minecraft/auth/AccountList.h"
#include "minecraft/auth/MinecraftAccount.h"
#include "ui/dialogs/ElyByLoginDialog.h"
#include "ui/dialogs/MSALoginDialog.h"
#include "ui_LoginWizardPage.h"

#include "Application.h"

LoginWizardPage::LoginWizardPage(QWidget* parent) : BaseWizardPage(parent), ui(new Ui::LoginWizardPage)
{
    ui->setupUi(this);

    ui->accountTypeCombo->addItem(tr("Local survivor (offline)"));
    ui->accountTypeCombo->addItem(tr("Ely.by account"));
    ui->accountTypeCombo->addItem(tr("Microsoft account"));

    connect(ui->accountTypeCombo, &QComboBox::currentIndexChanged, this, &LoginWizardPage::on_accountTypeCombo_currentIndexChanged);
    connect(ui->loginButton, &QPushButton::clicked, this, &LoginWizardPage::on_loginButton_clicked);
    connect(ui->skipButton, &QPushButton::clicked, this, &LoginWizardPage::on_skipButton_clicked);

    if (~APPLICATION->capabilities() & Application::SupportsMSA) {
        ui->accountTypeCombo->setItemData(2, 0, Qt::UserRole - 1);
    }
}

LoginWizardPage::~LoginWizardPage()
{
    delete ui;
}

void LoginWizardPage::initializePage()
{
    m_accountAdded = false;
    m_skipped = false;
    updateUiForSelection();
}

bool LoginWizardPage::validatePage()
{
    if (m_accountAdded || m_skipped) {
        return true;
    }

    if (selectedType() == DesiredAccountType::Offline) {
        return addSelectedAccount();
    }

    return true;
}

void LoginWizardPage::retranslate()
{
    ui->retranslateUi(this);

    int idx = ui->accountTypeCombo->currentIndex();
    ui->accountTypeCombo->clear();
    ui->accountTypeCombo->addItem(tr("Local survivor (offline)"));
    ui->accountTypeCombo->addItem(tr("Ely.by account"));
    ui->accountTypeCombo->addItem(tr("Microsoft account"));
    ui->accountTypeCombo->setCurrentIndex(idx < 0 ? 0 : idx);

    if (~APPLICATION->capabilities() & Application::SupportsMSA) {
        ui->accountTypeCombo->setItemData(2, 0, Qt::UserRole - 1);
    }

    updateUiForSelection();
}

LoginWizardPage::DesiredAccountType LoginWizardPage::selectedType() const
{
    switch (ui->accountTypeCombo->currentIndex()) {
        case 1:
            return DesiredAccountType::ElyBy;
        case 2:
            return DesiredAccountType::Microsoft;
        case 0:
        default:
            return DesiredAccountType::Offline;
    }
}

bool LoginWizardPage::addSelectedAccount()
{
    MinecraftAccountPtr account;

    switch (selectedType()) {
        case DesiredAccountType::Offline: {
            auto name = ui->offlineNameEdit->text().trimmed();
            if (name.isEmpty()) {
                return false;
            }
            account = MinecraftAccount::createOffline(name);
            if (account) {
                account->login()->start();
            }
            break;
        }
        case DesiredAccountType::ElyBy: {
            wizard()->hide();
            account = ElyByLoginDialog::newAccount(nullptr);
            wizard()->show();
            break;
        }
        case DesiredAccountType::Microsoft: {
            wizard()->hide();
            account = MSALoginDialog::newAccount(nullptr);
            wizard()->show();
            break;
        }
    }

    if (!account) {
        return false;
    }

    APPLICATION->accounts()->addAccount(account);
    APPLICATION->accounts()->setDefaultAccount(account);
    m_accountAdded = true;

    if (wizard()->currentId() == wizard()->pageIds().last()) {
        wizard()->accept();
    } else {
        wizard()->next();
    }

    return true;
}

void LoginWizardPage::updateUiForSelection()
{
    const bool isOffline = selectedType() == DesiredAccountType::Offline;
    ui->offlineNameEdit->setVisible(isOffline);
    ui->skipButton->setVisible(isOffline);
}

void LoginWizardPage::on_loginButton_clicked()
{
    addSelectedAccount();
}

void LoginWizardPage::on_skipButton_clicked()
{
    m_skipped = true;
    if (wizard()->currentId() == wizard()->pageIds().last()) {
        wizard()->accept();
    } else {
        wizard()->next();
    }
}

void LoginWizardPage::on_accountTypeCombo_currentIndexChanged([[maybe_unused]] int index)
{
    updateUiForSelection();
}
