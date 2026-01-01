#include "LoginWizardPage.h"
#include "minecraft/auth/AccountList.h"
#include "ui/dialogs/MSALoginDialog.h"
#include "ui/dialogs/ChooseOfflineNameDialog.h"
#include "ui_LoginWizardPage.h"

#include "Application.h"

LoginWizardPage::LoginWizardPage(QWidget* parent) : BaseWizardPage(parent), ui(new Ui::LoginWizardPage)
{
    ui->setupUi(this);
}

LoginWizardPage::~LoginWizardPage()
{
    delete ui;
}

void LoginWizardPage::initializePage() {}

bool LoginWizardPage::validatePage()
{
    return true;
}

void LoginWizardPage::retranslate()
{
    ui->retranslateUi(this);
}

void LoginWizardPage::on_pushButton_clicked()
{
    wizard()->hide();
    auto account = MSALoginDialog::newAccount(nullptr);
    wizard()->show();
    if (account) {
        APPLICATION->accounts()->addAccount(account);
        APPLICATION->accounts()->setDefaultAccount(account);
        if (wizard()->currentId() == wizard()->pageIds().last()) {
            wizard()->accept();
        } else {
            wizard()->next();
        }
    }
}

void LoginWizardPage::on_pushButtonOffline_clicked()
{
    ChooseOfflineNameDialog dialog(tr("Please enter your desired username to add your offline account."), this);
    if (dialog.exec() == QDialog::Accepted) {
        if (const MinecraftAccountPtr account = MinecraftAccount::createOffline(dialog.getUsername())) {
            account->login()->start();
            APPLICATION->accounts()->addAccount(account);
            APPLICATION->accounts()->setDefaultAccount(account);
            if (wizard()->currentId() == wizard()->pageIds().last()) {
                wizard()->accept();
            } else {
                wizard()->next();
            }
        }
    }
}
