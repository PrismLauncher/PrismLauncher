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
    m_account = MinecraftAccount::createBlankMSA();
    m_loginTask = m_account->loginMSA();
    connect(m_loginTask.get(), &Task::failed, this, &MSALoginDialog::onTaskFailed);
    connect(m_loginTask.get(), &Task::succeeded, this, &MSALoginDialog::onTaskSucceeded);
    connect(m_loginTask.get(), &Task::status, this, &MSALoginDialog::onTaskStatus);
    connect(m_loginTask.get(), &Task::progress, this, &MSALoginDialog::onTaskProgress);
    connect(m_loginTask.get(), &AccountTask::showVerificationUriAndCode, this, &MSALoginDialog::showVerificationUriAndCode);
    connect(m_loginTask.get(), &AccountTask::hideVerificationUriAndCode, this, &MSALoginDialog::hideVerificationUriAndCode);
    connect(&m_externalLoginTimer, &QTimer::timeout, this, &MSALoginDialog::externalLoginTick);
    m_loginTask->start();

    return QDialog::exec();
}


MSALoginDialog::~MSALoginDialog()
{
    delete ui;
}

void MSALoginDialog::externalLoginTick() {
    m_externalLoginElapsed++;
    ui->progressBar->setValue(m_externalLoginElapsed);
    ui->progressBar->repaint();

    if(m_externalLoginElapsed >= m_externalLoginTimeout) {
        m_externalLoginTimer.stop();
    }
}


void MSALoginDialog::showVerificationUriAndCode(const QUrl& uri, const QString& code, int expiresIn) {
    m_externalLoginElapsed = 0;
    m_externalLoginTimeout = expiresIn;

    m_externalLoginTimer.setInterval(1000);
    m_externalLoginTimer.setSingleShot(false);
    m_externalLoginTimer.start();

    ui->progressBar->setMaximum(expiresIn);
    ui->progressBar->setValue(m_externalLoginElapsed);

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
    m_externalLoginTimer.stop();
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
        return dlg.m_account;
    }
    return nullptr;
}
