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

#include "CustomYggdrasilLoginDialog.h"
#include "ui_CustomYggdrasilLoginDialog.h"

#include "minecraft/auth/AccountTask.h"

#include <QtWidgets/QPushButton>

CustomYggdrasilLoginDialog::CustomYggdrasilLoginDialog(QWidget *parent) : QDialog(parent), ui(new Ui::CustomYggdrasilLoginDialog)
{
    ui->setupUi(this);
    ui->progressBar->setVisible(false);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

CustomYggdrasilLoginDialog::~CustomYggdrasilLoginDialog()
{
    delete ui;
}

QString CustomYggdrasilLoginDialog::fixUrl(QString url)
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
void CustomYggdrasilLoginDialog::accept()
{
    setUserInputsEnabled(false);
    ui->progressBar->setVisible(true);

    // Setup the login task and start it
    m_account = MinecraftAccount::createFromUsernameCustomYggdrasil(
        ui->userTextBox->text(),
        CustomYggdrasilLoginDialog::fixUrl(ui->authServerTextBox->text()),
        CustomYggdrasilLoginDialog::fixUrl(ui->accountServerTextBox->text()),
        CustomYggdrasilLoginDialog::fixUrl(ui->sessionServerTextBox->text()),
        CustomYggdrasilLoginDialog::fixUrl(ui->servicesServerTextBox->text())
    );

    m_loginTask = m_account->loginCustomYggdrasil(ui->passTextBox->text());
    connect(m_loginTask.get(), &Task::failed, this, &CustomYggdrasilLoginDialog::onTaskFailed);
    connect(m_loginTask.get(), &Task::succeeded, this, &CustomYggdrasilLoginDialog::onTaskSucceeded);
    connect(m_loginTask.get(), &Task::status, this, &CustomYggdrasilLoginDialog::onTaskStatus);
    connect(m_loginTask.get(), &Task::progress, this, &CustomYggdrasilLoginDialog::onTaskProgress);
    m_loginTask->start();
}

void CustomYggdrasilLoginDialog::setUserInputsEnabled(bool enable)
{
    ui->userTextBox->setEnabled(enable);
    ui->passTextBox->setEnabled(enable);
    ui->authServerTextBox->setEnabled(enable);
    ui->accountServerTextBox->setEnabled(enable);
    ui->sessionServerTextBox->setEnabled(enable);
    ui->servicesServerTextBox->setEnabled(enable);
    ui->buttonBox->setEnabled(enable);
}

// Enable the OK button only when all textboxes contain something.
void CustomYggdrasilLoginDialog::on_userTextBox_textEdited(const QString &newText)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)
        ->setEnabled(!newText.isEmpty() &&
                     !ui->passTextBox->text().isEmpty() &&
                     !ui->authServerTextBox->text().isEmpty() &&
                     !ui->accountServerTextBox->text().isEmpty() &&
                     !ui->sessionServerTextBox->text().isEmpty() &&
                     !ui->servicesServerTextBox->text().isEmpty());

}
void CustomYggdrasilLoginDialog::on_passTextBox_textEdited(const QString &newText)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)
        ->setEnabled(!newText.isEmpty() && !ui->userTextBox->text().isEmpty());
}

void CustomYggdrasilLoginDialog::onTaskFailed(const QString &reason)
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

void CustomYggdrasilLoginDialog::onTaskSucceeded()
{
    QDialog::accept();
}

void CustomYggdrasilLoginDialog::onTaskStatus(const QString &status)
{
    ui->label->setText(status);
}

void CustomYggdrasilLoginDialog::onTaskProgress(qint64 current, qint64 total)
{
    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(current);
}

// Public interface
MinecraftAccountPtr CustomYggdrasilLoginDialog::newAccount(QWidget *parent, QString msg)
{
    CustomYggdrasilLoginDialog dlg(parent);
    dlg.ui->label->setText(msg);
    if (dlg.exec() == QDialog::Accepted)
    {
        return dlg.m_account;
    }
    return nullptr;
}
