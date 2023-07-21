/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "AuthlibInjectorLoginDialog.h"
#include "ui_AuthlibInjectorLoginDialog.h"
#include "ui/dialogs/CustomMessageBox.h"

#include "minecraft/auth/AccountTask.h"

#include <QtWidgets/QPushButton>

AuthlibInjectorLoginDialog::AuthlibInjectorLoginDialog(QWidget *parent) : QDialog(parent), ui(new Ui::AuthlibInjectorLoginDialog)
{
    ui->setupUi(this);
    ui->progressBar->setVisible(false);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

AuthlibInjectorLoginDialog::~AuthlibInjectorLoginDialog()
{
    delete ui;
}

QString AuthlibInjectorLoginDialog::fixUrl(QString url)
{
    QString fixed(url);
    if (!fixed.contains("://")) {
        fixed.prepend("https://");
    }
    if (fixed.endsWith("/")) {
        fixed = fixed.left(fixed.size() - 1);
    }
    return fixed;
}

// Stage 1: User interaction
void AuthlibInjectorLoginDialog::accept()
{
    auto fixedAuthlibInjectorUrl = AuthlibInjectorLoginDialog::fixUrl(ui->authlibInjectorTextBox->text());

    auto response = CustomMessageBox::selectable(this, QObject::tr("Confirm account creation"),
        QObject::tr(
            "Warning: you are about to send the username and password you entered to an "
            "unofficial, third-party authentication server:\n"
            "%1\n\n"
            "Never use your Mojang or Microsoft password for a third-party account!\n\n"
            "Are you sure you want to proceed?"
        ).arg(fixedAuthlibInjectorUrl),
        QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)->exec();
    if (response != QMessageBox::Yes)
        return;

    setUserInputsEnabled(false);
    ui->progressBar->setVisible(true);

    // Setup the login task and start it
    m_account = MinecraftAccount::createFromUsernameAuthlibInjector(
        ui->userTextBox->text(),
        fixedAuthlibInjectorUrl
    );

    m_loginTask = m_account->loginAuthlibInjector(ui->passTextBox->text());
    connect(m_loginTask.get(), &Task::failed, this, &AuthlibInjectorLoginDialog::onTaskFailed);
    connect(m_loginTask.get(), &Task::succeeded, this, &AuthlibInjectorLoginDialog::onTaskSucceeded);
    connect(m_loginTask.get(), &Task::status, this, &AuthlibInjectorLoginDialog::onTaskStatus);
    connect(m_loginTask.get(), &Task::progress, this, &AuthlibInjectorLoginDialog::onTaskProgress);
    m_loginTask->start();
}

void AuthlibInjectorLoginDialog::setUserInputsEnabled(bool enable)
{
    ui->userTextBox->setEnabled(enable);
    ui->passTextBox->setEnabled(enable);
    ui->authlibInjectorTextBox->setEnabled(enable);
    ui->buttonBox->setEnabled(enable);
}

// Enable the OK button only when all textboxes contain something.
void AuthlibInjectorLoginDialog::on_userTextBox_textEdited(const QString &newText)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)
        ->setEnabled(!newText.isEmpty() &&
                     !ui->passTextBox->text().isEmpty() &&
                     !ui->authlibInjectorTextBox->text().isEmpty());

}
void AuthlibInjectorLoginDialog::on_passTextBox_textEdited(const QString &newText)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)
        ->setEnabled(!newText.isEmpty() &&
                     !ui->passTextBox->text().isEmpty() &&
                     !ui->authlibInjectorTextBox->text().isEmpty());
}
void AuthlibInjectorLoginDialog::on_authlibInjectorTextBox_textEdited(const QString &newText)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)
        ->setEnabled(!newText.isEmpty() &&
                     !ui->passTextBox->text().isEmpty() &&
                     !ui->authlibInjectorTextBox->text().isEmpty());
}

void AuthlibInjectorLoginDialog::onTaskFailed(const QString &reason)
{
    // Set message
    auto lines = reason.split('\n');
    QString processed;
    for(auto line: lines) {
        if(line.size()) {
            processed += "<font color='red'>" + line + "</font><br />";
        }
        else {
            processed += "<br />";
        }
    }
    ui->label->setText(processed);

    // Re-enable user-interaction
    setUserInputsEnabled(true);
    ui->progressBar->setVisible(false);
}

void AuthlibInjectorLoginDialog::onTaskSucceeded()
{
    QDialog::accept();
}

void AuthlibInjectorLoginDialog::onTaskStatus(const QString &status)
{
    ui->label->setText(status);
}

void AuthlibInjectorLoginDialog::onTaskProgress(qint64 current, qint64 total)
{
    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(current);
}

// Public interface
MinecraftAccountPtr AuthlibInjectorLoginDialog::newAccount(QWidget *parent, QString msg)
{
    AuthlibInjectorLoginDialog dlg(parent);
    dlg.ui->label->setText(msg);
    if (dlg.exec() == QDialog::Accepted)
    {
        return dlg.m_account;
    }
    return nullptr;
}
