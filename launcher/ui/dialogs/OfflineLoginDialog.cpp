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

#include "OfflineLoginDialog.h"
#include "ui_OfflineLoginDialog.h"

#include "minecraft/auth/AccountTask.h"

#include <QtWidgets/QPushButton>

OfflineLoginDialog::OfflineLoginDialog(QWidget *parent) : QDialog(parent), ui(new Ui::OfflineLoginDialog)
{
    ui->setupUi(this);
    ui->progressBar->setVisible(false);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

OfflineLoginDialog::~OfflineLoginDialog()
{
    delete ui;
}

// Stage 1: User interaction
void OfflineLoginDialog::accept()
{
    setUserInputsEnabled(false);
    ui->progressBar->setVisible(true);

    // Setup the login task and start it
    m_account = MinecraftAccount::createFromUsername(ui->userTextBox->text());
    m_loginTask = m_account->login("TODO: create offline mode account flow");
    connect(m_loginTask.get(), &Task::failed, this, &OfflineLoginDialog::onTaskFailed);
    connect(m_loginTask.get(), &Task::succeeded, this, &OfflineLoginDialog::onTaskSucceeded);
    connect(m_loginTask.get(), &Task::status, this, &OfflineLoginDialog::onTaskStatus);
    connect(m_loginTask.get(), &Task::progress, this, &OfflineLoginDialog::onTaskProgress);
    m_loginTask->start();
}

void OfflineLoginDialog::setUserInputsEnabled(bool enable)
{
    ui->userTextBox->setEnabled(enable);
    ui->buttonBox->setEnabled(enable);
}

// Enable the OK button only when the textbox contains something.
void OfflineLoginDialog::on_userTextBox_textEdited(const QString &newText)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)
        ->setEnabled(!newText.isEmpty());
}

void OfflineLoginDialog::onTaskFailed(const QString &reason)
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

void OfflineLoginDialog::onTaskSucceeded()
{
    QDialog::accept();
}

void OfflineLoginDialog::onTaskStatus(const QString &status)
{
    ui->label->setText(status);
}

void OfflineLoginDialog::onTaskProgress(qint64 current, qint64 total)
{
    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(current);
}

// Public interface
MinecraftAccountPtr OfflineLoginDialog::newAccount(QWidget *parent, QString msg)
{
    OfflineLoginDialog dlg(parent);
    dlg.ui->label->setText(msg);
    if (dlg.exec() == QDialog::Accepted)
    {
        return dlg.m_account;
    }
    return 0;
}
