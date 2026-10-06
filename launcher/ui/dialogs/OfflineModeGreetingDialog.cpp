#include "OfflineModeGreetingDialog.h"
#include "ui_OfflineModeGreetingDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

#include "Application.h"
#include "minecraft/auth/MinecraftAccount.h"
#include "minecraft/auth/AccountList.h"
#include "ui/dialogs/ChooseOfflineNameDialog.h"
#include "ui/dialogs/MSALoginDialog.h"
#include "ui/dialogs/CustomMessageBox.h"

OfflineModeGreetingDialog::OfflineModeGreetingDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::OfflineModeGreetingDialog), m_app(APPLICATION)
{
    setWindowTitle(tr("Welcome to Prism Launcher"));
    setMinimumWidth(600);
    setMinimumHeight(400);
    setWindowModality(Qt::ApplicationModal);

    auto mainLayout = new QVBoxLayout(this);

    // Header
    auto headerLabel = new QLabel(
        tr("<h2>Welcome to Prism Launcher</h2>"
           "<p>Choose how you want to play Minecraft:</p>"));
    headerLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(headerLabel);

    // Separator
    auto separator1 = new QFrame();
    separator1->setFrameShape(QFrame::HLine);
    separator1->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(separator1);

    // Option 1: Microsoft Account
    auto option1Label = new QLabel(
        tr("<b>Option 1: Microsoft Account</b><br>"
           "• Play online with your Microsoft account<br>"
           "• Access Realms and multiplayer servers<br>"
           "• Use your official Minecraft profile"));
    option1Label->setWordWrap(true);
    mainLayout->addWidget(option1Label);

    auto microsoftButton = new QPushButton(tr("Add Microsoft Account"));
    microsoftButton->setMinimumHeight(40);
    connect(microsoftButton, &QPushButton::clicked, this, &OfflineModeGreetingDialog::on_addMicrosoftButton_clicked);
    mainLayout->addWidget(microsoftButton);

    // Separator
    auto separator2 = new QFrame();
    separator2->setFrameShape(QFrame::HLine);
    separator2->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(separator2);

    // Option 2: Offline Mode
    auto option2Label = new QLabel(
        tr("<b>Option 2: Offline Account (No Login Required)</b><br>"
           "• Play completely offline, no account needed<br>"
           "• Single-player and LAN multiplayer<br>"
           "• Full Minecraft features without restrictions<br>"
           "• Perfect for testing and local development"));
    option2Label->setWordWrap(true);
    mainLayout->addWidget(option2Label);

    auto offlineButton = new QPushButton(tr("Create Offline Account"));
    offlineButton->setMinimumHeight(40);
    offlineButton->setStyleSheet(
        "QPushButton { "
        "background-color: #7c3aed; "
        "color: white; "
        "border: none; "
        "border-radius: 8px; "
        "font-weight: bold; "
        "}"
        "QPushButton:hover { background-color: #8b5cf6; }");
    connect(offlineButton, &QPushButton::clicked, this, &OfflineModeGreetingDialog::on_addOfflineButton_clicked);
    mainLayout->addWidget(offlineButton);

    // Separator
    auto separator3 = new QFrame();
    separator3->setFrameShape(QFrame::HLine);
    separator3->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(separator3);

    // Skip button
    auto skipButton = new QPushButton(tr("Skip for Now"));
    skipButton->setMinimumHeight(35);
    connect(skipButton, &QPushButton::clicked, this, &OfflineModeGreetingDialog::on_skipButton_clicked);
    mainLayout->addWidget(skipButton);

    setLayout(mainLayout);
}

OfflineModeGreetingDialog::~OfflineModeGreetingDialog()
{
    delete ui;
}

void OfflineModeGreetingDialog::on_addMicrosoftButton_clicked()
{
    auto accountList = m_app->accounts();
    auto account = MSALoginDialog::newAccount(this);
    if (account) {
        accountList->addAccount(account);
        if (accountList->count() == 1) {
            accountList->setDefaultAccount(account);
        }
        CustomMessageBox::selectable(
            this,
            tr("Account Added"),
            tr("Microsoft account added successfully!\nYou can now play online."),
            QMessageBox::Information,
            QMessageBox::Ok,
            QMessageBox::Ok
        )->exec();
        accept();
    }
}

void OfflineModeGreetingDialog::on_addOfflineButton_clicked()
{
    ChooseOfflineNameDialog dialog(
        tr("Enter a username for your offline account.\n\n"
           "This allows you to play Minecraft without any online authentication."),
        this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    auto accountList = m_app->accounts();
    if (const auto account = MinecraftAccount::createOffline(dialog.getUsername())) {
        accountList->addAccount(account);
        if (accountList->count() == 1) {
            accountList->setDefaultAccount(account);
        }
        CustomMessageBox::selectable(
            this,
            tr("Offline Account Created"),
            tr("Successfully created offline account: %1\n\n"
               "You can now play Minecraft without any online authentication.").arg(dialog.getUsername()),
            QMessageBox::Information,
            QMessageBox::Ok,
            QMessageBox::Ok
        )->exec();
        accept();
    }
}

void OfflineModeGreetingDialog::on_skipButton_clicked()
{
    reject();
}
