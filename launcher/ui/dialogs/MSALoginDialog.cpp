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
#include "Application.h"

#include "ui_MSALoginDialog.h"

#include "DesktopServices.h"
#include "minecraft/auth/AuthFlow.h"

#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QPainter>
#include <QPixmap>
#include <QSize>
#include <QUrl>
#include <QtWidgets/QPushButton>

#include "qrencode.h"

MSALoginDialog::MSALoginDialog(QWidget* parent) : QDialog(parent), ui(new Ui::MSALoginDialog)
{
    ui->setupUi(this);

    // make font monospace
    QFont font;
    font.setPixelSize(ui->code->fontInfo().pixelSize());
    font.setFamily(APPLICATION->settings()->get("ConsoleFont").toString());
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    ui->code->setFont(font);

    connect(ui->copyCode, &QPushButton::clicked, this, [this] { QApplication::clipboard()->setText(ui->code->text()); });
    connect(ui->loginButton, &QPushButton::clicked, this, [this] {
        if (m_url.isValid()) {
            if (!DesktopServices::openUrl(m_url)) {
                QApplication::clipboard()->setText(m_url.toString());
            }
        }
    });

    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
}

int MSALoginDialog::exec()
{
    // Setup the login task and start it
    m_account = MinecraftAccount::createBlankMSA();
    m_authflow_task = m_account->login(false);
    connect(m_authflow_task.get(), &Task::failed, this, &MSALoginDialog::onTaskFailed);
    connect(m_authflow_task.get(), &Task::succeeded, this, &QDialog::accept);
    connect(m_authflow_task.get(), &Task::aborted, this, &MSALoginDialog::reject);
    connect(m_authflow_task.get(), &Task::status, this, &MSALoginDialog::onAuthFlowStatus);
    connect(m_authflow_task.get(), &AuthFlow::authorizeWithBrowser, this, &MSALoginDialog::authorizeWithBrowser);
    connect(m_authflow_task.get(), &AuthFlow::authorizeWithBrowserWithExtra, this, &MSALoginDialog::authorizeWithBrowserWithExtra);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, m_authflow_task.get(), &Task::abort);

    m_devicecode_task.reset(new AuthFlow(m_account->accountData(), AuthFlow::Action::DeviceCode));
    connect(m_devicecode_task.get(), &Task::failed, this, &MSALoginDialog::onTaskFailed);
    connect(m_devicecode_task.get(), &Task::succeeded, this, &QDialog::accept);
    connect(m_devicecode_task.get(), &Task::aborted, this, &MSALoginDialog::reject);
    connect(m_devicecode_task.get(), &Task::status, this, &MSALoginDialog::onDeviceFlowStatus);
    connect(m_devicecode_task.get(), &AuthFlow::authorizeWithBrowser, this, &MSALoginDialog::authorizeWithBrowser);
    connect(m_devicecode_task.get(), &AuthFlow::authorizeWithBrowserWithExtra, this, &MSALoginDialog::authorizeWithBrowserWithExtra);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, m_devicecode_task.get(), &Task::abort);
    QMetaObject::invokeMethod(m_authflow_task.get(), &Task::start, Qt::QueuedConnection);
    QMetaObject::invokeMethod(m_devicecode_task.get(), &Task::start, Qt::QueuedConnection);

    return QDialog::exec();
}

MSALoginDialog::~MSALoginDialog()
{
    delete ui;
}

void MSALoginDialog::onTaskFailed(QString reason)
{
    // Set message
    m_authflow_task->disconnect();
    m_devicecode_task->disconnect();
    ui->stackedWidget->setCurrentIndex(0);
    auto lines = reason.split('\n');
    QString processed;
    for (auto line : lines) {
        if (line.size()) {
            processed += "<font color='red'>" + line + "</font><br />";
        } else {
            processed += "<br />";
        }
    }
    ui->status->setText(processed);
    auto task = m_authflow_task;
    if (task->failReason().isEmpty()) {
        task = m_devicecode_task;
    }
    if (task) {
        ui->loadingLabel->setText(task->getStatus());
    }
    disconnect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, m_authflow_task.get(), &Task::abort);
    disconnect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, m_devicecode_task.get(), &Task::abort);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &MSALoginDialog::reject);
}

void MSALoginDialog::authorizeWithBrowser(const QUrl& url)
{
    ui->stackedWidget2->setCurrentIndex(1);
    ui->stackedWidget2->adjustSize();
    ui->stackedWidget2->updateGeometry();
    this->adjustSize();
    ui->loginButton->setToolTip(QString("<div style='width: 200px;'>%1</div>").arg(url.toString()));
    m_url = url;
}

void paintQR(QPainter& painter, const QSize canvasSize, const QString& data, QColor fg)
{
    const auto* qr = QRcode_encodeString(data.toUtf8().constData(), 0, QRecLevel::QR_ECLEVEL_M, QRencodeMode::QR_MODE_8, 1);
    if (!qr) {
        qWarning() << "Unable to encode" << data << "as QR code";
        return;
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(fg);

    // Make sure the QR code fits in the canvas with some padding
    const auto qrSize = qr->width;
    const auto canvasWidth = canvasSize.width();
    const auto canvasHeight = canvasSize.height();
    const auto scale = 0.8 * std::min(canvasWidth / qrSize, canvasHeight / qrSize);

    // Find an offset to center it in the canvas
    const auto offsetX = (canvasWidth - qrSize * scale) / 2;
    const auto offsetY = (canvasHeight - qrSize * scale) / 2;

    for (int y = 0; y < qrSize; y++) {
        for (int x = 0; x < qrSize; x++) {
            auto shouldFillIn = qr->data[y * qrSize + x] & 1;
            if (shouldFillIn) {
                QRectF r(offsetX + x * scale, offsetY + y * scale, scale, scale);
                painter.drawRects(&r, 1);
            }
        }
    }
}

void MSALoginDialog::authorizeWithBrowserWithExtra(QString url, QString code, [[maybe_unused]] int expiresIn)
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->stackedWidget->adjustSize();
    ui->stackedWidget->updateGeometry();
    this->adjustSize();

    const auto linkString = QString("<a href=\"%1\">%2</a>").arg(url, url);
    if (url == "https://www.microsoft.com/link" && !code.isEmpty()) {
        url += QString("?otc=%1").arg(code);
    }
    ui->code->setText(code);

    auto size = QSize(150, 150);
    QPixmap pixmap(size);
    pixmap.fill(Qt::white);

    QPainter painter(&pixmap);
    paintQR(painter, size, url, Qt::black);

    // Set the generated pixmap to the label
    ui->qr->setPixmap(pixmap);

    ui->qrMessage->setText(tr("Open %1 or scan the QR and enter the above code if needed.").arg(linkString));
}

void MSALoginDialog::onDeviceFlowStatus(QString status)
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->stackedWidget->adjustSize();
    ui->stackedWidget->updateGeometry();
    this->adjustSize();
    ui->status->setText(status);
}

void MSALoginDialog::onAuthFlowStatus(QString status)
{
    ui->stackedWidget2->setCurrentIndex(0);
    ui->stackedWidget2->adjustSize();
    ui->stackedWidget2->updateGeometry();
    this->adjustSize();
    ui->status2->setText(status);
}

// Public interface
MinecraftAccountPtr MSALoginDialog::newAccount(QWidget* parent)
{
    MSALoginDialog dlg(parent);
    if (dlg.exec() == QDialog::Accepted) {
        return dlg.m_account;
    }
    return nullptr;
}
