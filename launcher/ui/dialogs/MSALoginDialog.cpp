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
#include "settings/SettingsObject.h"

#include "ui_MSALoginDialog.h"

#include "DesktopServices.h"
#include "minecraft/auth/AuthFlow.h"

#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QSize>
#include <QUrl>
#include <QtWidgets/QPushButton>
#include <span>

#include "qrencode.h"

MSALoginDialog::MSALoginDialog(QWidget* parent) : QDialog(parent), m_ui(new Ui::MSALoginDialog)
{
    m_ui->setupUi(this);

    // make font monospace
    QFont font;
    font.setPixelSize(m_ui->code->fontInfo().pixelSize());
    font.setFamily(APPLICATION->settings()->get("ConsoleFont").toString());
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    m_ui->code->setFont(font);

    connect(m_ui->copyCode, &QPushButton::clicked, this, [this] { QApplication::clipboard()->setText(m_ui->code->text()); });
    connect(m_ui->loginButton, &QPushButton::clicked, this, [this] {
        if (m_url.isValid()) {
            if (!DesktopServices::openUrl(m_url)) {
                QApplication::clipboard()->setText(m_url.toString());
            }
        }
    });

    m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
}

int MSALoginDialog::exec()
{
    // Setup the login task and start it
    m_account = MinecraftAccount::createBlankMSA();
    m_authflowTask = m_account->login(false);
    connect(m_authflowTask.get(), &Task::failed, this, &MSALoginDialog::onTaskFailed);
    connect(m_authflowTask.get(), &Task::succeeded, this, &QDialog::accept);
    connect(m_authflowTask.get(), &Task::aborted, this, &MSALoginDialog::reject);
    connect(m_authflowTask.get(), &Task::status, this, &MSALoginDialog::onAuthFlowStatus);
    connect(m_authflowTask.get(), &AuthFlow::authorizeWithBrowser, this, &MSALoginDialog::authorizeWithBrowser);
    connect(m_authflowTask.get(), &AuthFlow::authorizeWithBrowserWithExtra, this, &MSALoginDialog::authorizeWithBrowserWithExtra);
    connect(m_ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, m_authflowTask.get(), &Task::abort);

    m_devicecodeTask.reset(new AuthFlow(m_account->accountData(), AuthFlow::Action::DeviceCode));
    connect(m_devicecodeTask.get(), &Task::failed, this, &MSALoginDialog::onTaskFailed);
    connect(m_devicecodeTask.get(), &Task::succeeded, this, &QDialog::accept);
    connect(m_devicecodeTask.get(), &Task::aborted, this, &MSALoginDialog::reject);
    connect(m_devicecodeTask.get(), &Task::status, this, &MSALoginDialog::onDeviceFlowStatus);
    connect(m_devicecodeTask.get(), &AuthFlow::authorizeWithBrowser, this, &MSALoginDialog::authorizeWithBrowser);
    connect(m_devicecodeTask.get(), &AuthFlow::authorizeWithBrowserWithExtra, this, &MSALoginDialog::authorizeWithBrowserWithExtra);
    connect(m_ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, m_devicecodeTask.get(), &Task::abort);
    QMetaObject::invokeMethod(m_authflowTask.get(), &Task::start, Qt::QueuedConnection);
    QMetaObject::invokeMethod(m_devicecodeTask.get(), &Task::start, Qt::QueuedConnection);

    return QDialog::exec();
}

MSALoginDialog::~MSALoginDialog()
{
    delete m_ui;
}

void MSALoginDialog::onTaskFailed(const QString& reason)
{
    // Set message
    m_authflowTask->disconnect();
    m_devicecodeTask->disconnect();
    m_ui->stackedWidget->setCurrentIndex(0);
    auto lines = reason.split('\n');
    QString processed;
    for (const auto& line : lines) {
        if (line.size() != 0) {
            processed += "<font color='red'>" + line + "</font><br />";
        } else {
            processed += "<br />";
        }
    }
    m_ui->status->setText(processed);
    auto task = m_authflowTask;
    if (task->failReason().isEmpty()) {
        task = m_devicecodeTask;
    }
    if (task) {
        m_ui->loadingLabel->setText(task->getStatus());
    }
    disconnect(m_ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, m_authflowTask.get(), &Task::abort);
    disconnect(m_ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, m_devicecodeTask.get(), &Task::abort);
    connect(m_ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &MSALoginDialog::reject);
}

void MSALoginDialog::authorizeWithBrowser(const QUrl& url)
{
    m_ui->stackedWidget2->setCurrentIndex(1);
    m_ui->stackedWidget2->adjustSize();
    m_ui->stackedWidget2->updateGeometry();
    this->adjustSize();
    m_ui->loginButton->setToolTip(QString("<div style='width: 200px;'>%1</div>").arg(url.toString()));
    m_url = url;
}

namespace {
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
    const auto offsetX = (canvasWidth - (qrSize * scale)) / 2;
    const auto offsetY = (canvasHeight - (qrSize * scale)) / 2;

    for (int y = 0; y < qrSize; y++) {
        for (int x = 0; x < qrSize; x++) {
            auto shouldFillIn = qr->data[(y * qrSize) + x] & 1U;
            if (shouldFillIn != 0) {
                const QRectF r(offsetX + (x * scale), offsetY + (y * scale), scale, scale);
                painter.drawRects(&r, 1);
            }
        }
    }
}
}  // namespace

void MSALoginDialog::authorizeWithBrowserWithExtra(const QUrl& verificationUrl, const QString& code, const QUrl& completeVerificationUrl)
{
    m_ui->stackedWidget->setCurrentIndex(1);
    m_ui->stackedWidget->adjustSize();
    m_ui->stackedWidget->updateGeometry();
    this->adjustSize();

    auto url = verificationUrl.toString();
    if (completeVerificationUrl.isValid()) {
        url = completeVerificationUrl.toString();
    }

    const auto linkString = QString("<a href=\"%1\">%2</a>").arg(url, url);
    if (!completeVerificationUrl.isValid() && url == "https://www.microsoft.com/link" && !code.isEmpty()) {
        url += QString("?otc=%1").arg(code);
    }
    m_ui->code->setText(code);

    auto size = QSize(150, 150);
    QPixmap pixmap(size);
    pixmap.fill(Qt::white);

    QPainter painter(&pixmap);
    paintQR(painter, size, url, Qt::black);

    // Set the generated pixmap to the label
    m_ui->qr->setPixmap(pixmap);

    m_ui->qrMessage->setText(tr("Open %1 or scan the QR and enter the above code if needed.").arg(linkString));
}

void MSALoginDialog::onDeviceFlowStatus(const QString& status)
{
    m_ui->stackedWidget->setCurrentIndex(0);
    m_ui->stackedWidget->adjustSize();
    m_ui->stackedWidget->updateGeometry();
    this->adjustSize();
    m_ui->status->setText(status);
}

void MSALoginDialog::onAuthFlowStatus(const QString& status)
{
    m_ui->stackedWidget2->setCurrentIndex(0);
    m_ui->stackedWidget2->adjustSize();
    m_ui->stackedWidget2->updateGeometry();
    this->adjustSize();
    m_ui->status2->setText(status);
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
