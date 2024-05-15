// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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
#include "minecraft/auth/AuthFlow.h"

#include <QApplication>
#include <QClipboard>
#include <QUrl>
#include <QtWidgets/QPushButton>

MSALoginDialog::MSALoginDialog(QWidget* parent) : QDialog(parent), ui(new Ui::MSALoginDialog)
{
    ui->setupUi(this);

    ui->cancel->setEnabled(false);
    ui->link->setVisible(false);
    ui->copy->setVisible(false);

    connect(ui->cancel, &QPushButton::pressed, this, &QDialog::reject);
    connect(ui->copy, &QPushButton::pressed, this, &MSALoginDialog::copyUrl);
}

int MSALoginDialog::exec()
{
    // Setup the login task and start it
    m_account = MinecraftAccount::createBlankMSA();
    m_loginTask = m_account->login();
    connect(m_loginTask.get(), &Task::failed, this, &MSALoginDialog::onTaskFailed);
    connect(m_loginTask.get(), &Task::succeeded, this, &MSALoginDialog::onTaskSucceeded);
    connect(m_loginTask.get(), &Task::status, this, &MSALoginDialog::onTaskStatus);
    connect(m_loginTask.get(), &AuthFlow::authorizeWithBrowser, this, &MSALoginDialog::authorizeWithBrowser);
    m_loginTask->start();

    return QDialog::exec();
}

MSALoginDialog::~MSALoginDialog()
{
    delete ui;
}

void MSALoginDialog::onTaskFailed(const QString& reason)
{
    // Set message
    auto lines = reason.split('\n');
    QString processed;
    for (auto line : lines) {
        if (line.size()) {
            processed += "<font color='red'>" + line + "</font><br />";
        } else {
            processed += "<br />";
        }
    }
    ui->message->setText(processed);
}

void MSALoginDialog::onTaskSucceeded()
{
    QDialog::accept();
}

void MSALoginDialog::onTaskStatus(const QString& status)
{
    ui->message->setText(status);
    ui->cancel->setEnabled(false);
    ui->link->setVisible(false);
    ui->copy->setVisible(false);
}

// Public interface
MinecraftAccountPtr MSALoginDialog::newAccount(QWidget* parent, QString msg)
{
    MSALoginDialog dlg(parent);
    dlg.ui->message->setText(msg);
    if (dlg.exec() == QDialog::Accepted) {
        return dlg.m_account;
    }
    return nullptr;
}

void MSALoginDialog::authorizeWithBrowser(const QUrl& url)
{
    ui->cancel->setEnabled(true);
    ui->link->setVisible(true);
    ui->copy->setVisible(true);
    DesktopServices::openUrl(url);
    ui->link->setText(url.toDisplayString());
    ui->message->setText(
        tr("Browser opened to complete the login process."
           "<br /><br />"
           "If your browser hasn't opened, please manually open the bellow link in your browser:"));
}

void MSALoginDialog::copyUrl()
{
    QClipboard* cb = QApplication::clipboard();
    cb->setText(ui->link->text());
}
