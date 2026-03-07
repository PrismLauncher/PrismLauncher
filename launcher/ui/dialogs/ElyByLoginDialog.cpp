#include "ElyByLoginDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

ElyByLoginDialog::ElyByLoginDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Add Ely.by Account"));

    auto* layout = new QFormLayout(this);
    auto* info = new QLabel(tr("Sign in with your Ely.by credentials."), this);
    info->setWordWrap(true);
    layout->addRow(info);

    m_login = new QLineEdit(this);
    m_login->setPlaceholderText(tr("Email or username"));
    layout->addRow(tr("Login:"), m_login);

    m_password = new QLineEdit(this);
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setPlaceholderText(tr("Password"));
    layout->addRow(tr("Password:"), m_password);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    auto* loginButton = buttons->addButton(tr("Login"), QDialogButtonBox::AcceptRole);
    layout->addRow(buttons);

    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(loginButton, &QPushButton::clicked, this, &ElyByLoginDialog::tryLogin);
}

void ElyByLoginDialog::tryLogin()
{
    const auto login = m_login->text().trimmed();
    const auto password = m_password->text();
    if (login.isEmpty() || password.isEmpty()) {
        return;
    }
    accept();
}

MinecraftAccountPtr ElyByLoginDialog::newAccount(QWidget* parent)
{
    ElyByLoginDialog dlg(parent);
    if (dlg.exec() != QDialog::Accepted) {
        return nullptr;
    }

    auto account = MinecraftAccount::createBlankEly();
    account->accountData()->yggdrasilToken.extra["elyLogin"] = dlg.m_login->text().trimmed();
    account->accountData()->yggdrasilToken.extra["elyPassword"] = dlg.m_password->text();
    account->accountData()->yggdrasilToken.extra["clientToken"] = account->internalId();
    account->login()->start();

    if (account->accountState() == AccountState::Online) {
        return account;
    }

    return nullptr;
}
