// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "MSALoginDialog.h"
#include "ui_MSALoginDialog.h"

#include "DesktopServices.h"
#include "minecraft/auth/AccountTask.h"

#include <QtWidgets/QPushButton>
#include <QUrl>
#include <QApplication>
#include <QClipboard>

MSALoginDialog::MSALoginDialog(QWidget *parent) : QDialog(parent), ui(new Ui::MSALoginDialog)
{
    ui->setupUi(this);
    ui->progressBar->setVisible(false);
    ui->actionButton->setVisible(false);
    // ui->buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

int MSALoginDialog::exec() {
    setUserInputsEnabled(false);
    ui->progressBar->setVisible(true);

    // Setup the login task and start it
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_account = MinecraftAccount::createBlankMSA();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loginTask = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_account->loginMSA();
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loginTask.get(), &Task::failed, this, &MSALoginDialog::onTaskFailed);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loginTask.get(), &Task::succeeded, this, &MSALoginDialog::onTaskSucceeded);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loginTask.get(), &Task::status, this, &MSALoginDialog::onTaskStatus);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loginTask.get(), &Task::progress, this, &MSALoginDialog::onTaskProgress);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loginTask.get(), &AccountTask::showVerificationUriAndCode, this, &MSALoginDialog::showVerificationUriAndCode);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loginTask.get(), &AccountTask::hideVerificationUriAndCode, this, &MSALoginDialog::hideVerificationUriAndCode);
    connect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_externalLoginTimer, &QTimer::timeout, this, &MSALoginDialog::externalLoginTick);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loginTask->start();

    return QDialog::exec();
}


MSALoginDialog::~MSALoginDialog()
{
    delete ui;
}

void MSALoginDialog::externalLoginTick() {
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_externalLoginElapsed++;
    ui->progressBar->setValue(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_externalLoginElapsed);
    ui->progressBar->repaint();

    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_externalLoginElapsed >= hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_externalLoginTimeout) {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_externalLoginTimer.stop();
    }
}


void MSALoginDialog::showVerificationUriAndCode(const QUrl& uri, const QString& code, int expiresIn) {
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_externalLoginElapsed = 0;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_externalLoginTimeout = expiresIn;

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_externalLoginTimer.setInterval(1000);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_externalLoginTimer.setSingleShot(false);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_externalLoginTimer.start();

    ui->progressBar->setMaximum(expiresIn);
    ui->progressBar->setValue(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_externalLoginElapsed);

    QString urlString = uri.toString();
    QString linkString = QString("<a href=\"%1\">%2</a>").arg(urlString, urlString);
    ui->label->setText(tr("<p>Please open up %1 in a browser and put in the code <b>%2</b> to proceed with login.</p>").arg(linkString, code));
    ui->actionButton->setVisible(true);
    connect(ui->actionButton, &QPushButton::clicked, [=]() {
        DesktopServices::openUrl(uri);
        QClipboard* cb = QApplication::clipboard();
        cb->setText(code);
    });
}

void MSALoginDialog::hideVerificationUriAndCode() {
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_externalLoginTimer.stop();
    ui->actionButton->setVisible(false);
}

void MSALoginDialog::setUserInputsEnabled(bool enable)
{
    ui->buttonBox->setEnabled(enable);
}

void MSALoginDialog::onTaskFailed(const QString &reason)
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
    ui->actionButton->setVisible(false);
}

void MSALoginDialog::onTaskSucceeded()
{
    QDialog::accept();
}

void MSALoginDialog::onTaskStatus(const QString &status)
{
    ui->label->setText(status);
}

void MSALoginDialog::onTaskProgress(qint64 current, qint64 total)
{
    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(current);
}

// Public interface
MinecraftAccountPtr MSALoginDialog::newAccount(QWidget *parent, QString msg)
{
    MSALoginDialog dlg(parent);
    dlg.ui->label->setText(msg);
    if (dlg.exec() == QDialog::Accepted)
    {
        return dlg.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_account;
    }
    return nullptr;
}
