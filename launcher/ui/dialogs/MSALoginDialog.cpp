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
    ui->progressBar->setVisible(false);

    connect(ui->cancel, &QPushButton::pressed, this, &QDialog::reject);
    connect(ui->copy, &QPushButton::pressed, this, &MSALoginDialog::copyUrl);
}

int MSALoginDialog::exec()
{
    // Setup the login task and start it
    m_account = MinecraftAccount::createBlankMSA();
    m_task = m_account->login(m_using_device_code);
    connect(m_task.get(), &Task::failed, this, &MSALoginDialog::onTaskFailed);
    connect(m_task.get(), &Task::succeeded, this, &MSALoginDialog::onTaskSucceeded);
    connect(m_task.get(), &Task::status, this, &MSALoginDialog::onTaskStatus);
    connect(m_task.get(), &AuthFlow::authorizeWithBrowser, this, &MSALoginDialog::authorizeWithBrowser);
    connect(m_task.get(), &AuthFlow::authorizeWithBrowserWithExtra, this, &MSALoginDialog::authorizeWithBrowserWithExtra);
    connect(ui->cancel, &QPushButton::pressed, m_task.get(), &Task::abort);
    connect(&m_external_timer, &QTimer::timeout, this, &MSALoginDialog::externalLoginTick);
    m_task->start();

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
    ui->progressBar->setVisible(false);
}

// Public interface
MinecraftAccountPtr MSALoginDialog::newAccount(QWidget* parent, QString msg, bool usingDeviceCode)
{
    MSALoginDialog dlg(parent);
    dlg.m_using_device_code = usingDeviceCode;
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
           "If your browser hasn't opened, please manually open the below link in your browser:"));
}

void MSALoginDialog::copyUrl()
{
    QClipboard* cb = QApplication::clipboard();
    cb->setText(ui->link->text());
}

void MSALoginDialog::authorizeWithBrowserWithExtra(QString url, QString code, int expiresIn)
{
    m_external_elapsed = 0;
    m_external_timeout = expiresIn;

    m_external_timer.setInterval(1000);
    m_external_timer.setSingleShot(false);
    m_external_timer.start();

    ui->progressBar->setMaximum(expiresIn);
    ui->progressBar->setValue(m_external_elapsed);

    QString linkString = QString("<a href=\"%1\">%2</a>").arg(url, url);
    if (url == "https://www.microsoft.com/link" && !code.isEmpty()) {
        url += QString("?otc=%1").arg(code);
        ui->message->setText(tr("<p>Please login in the opened browser. If no browser was opened, please open up %1 in "
                                "a browser and put in the code <b>%2</b> to proceed with login.</p>")
                                 .arg(linkString, code));
    } else {
        ui->message->setText(
            tr("<p>Please open up %1 in a browser and put in the code <b>%2</b> to proceed with login.</p>").arg(linkString, code));
    }
    ui->cancel->setEnabled(true);
    ui->link->setVisible(true);
    ui->copy->setVisible(true);
    ui->progressBar->setVisible(true);
    DesktopServices::openUrl(url);
    ui->link->setText(code);
}

void MSALoginDialog::externalLoginTick()
{
    m_external_elapsed++;
    ui->progressBar->setValue(m_external_elapsed);
    ui->progressBar->repaint();

    if (m_external_elapsed >= m_external_timeout) {
        m_external_timer.stop();
    }
}