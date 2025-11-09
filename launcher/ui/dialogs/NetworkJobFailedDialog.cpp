// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2025 Octol1ttle <l1ttleofficial@outlook.com>
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
 */

#include "NetworkJobFailedDialog.h"

#include "ui_NetworkJobFailedDialog.h"

NetworkJobFailedDialog::NetworkJobFailedDialog(QString jobName, int attempts, int requests, int failed, QWidget* parent) : QDialog(parent), ui(new Ui::NetworkJobFailedDialog)
{
    ui->setupUi(this);
    ui->failLabel->setText(ui->failLabel->text().arg(jobName));
    if (failed == requests) {
        ui->requestCountLabel->setText(tr("All %1 requests have failed after %2 attempts").arg(failed).arg(attempts));
    } else if (failed < requests / 2) {
        ui->requestCountLabel->setText(tr("Out of %1 requests, %2 have failed after %3 attempts").arg(requests).arg(failed).arg(attempts));
    } else {
        ui->requestCountLabel->setText(tr("Out of %1 requests, only %2 succeeded after %3 attempts").arg(requests).arg(requests - failed).arg(attempts));
    }
    ui->detailsTable->setRowCount(failed);

    setShowDetails(failed < 5);

    connect(ui->dialogButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->dialogButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

NetworkJobFailedDialog::~NetworkJobFailedDialog()
{
    delete ui;
}

void NetworkJobFailedDialog::addFailedRequest(int row, QUrl url, QString error) const
{
    const auto urlItem = new QTableWidgetItem(url.toString());
    const auto errorItem = new QTableWidgetItem(error);
    ui->detailsTable->setItem(row, 0, urlItem);
    ui->detailsTable->setItem(row, 1, errorItem);
}

void NetworkJobFailedDialog::setShowDetails(bool showDetails)
{
    m_showDetails = showDetails;
    ui->detailsTable->setVisible(m_showDetails);
    ui->detailsButton->setText(!m_showDetails ? tr("Show details") : tr("Hide details"));
}

void NetworkJobFailedDialog::on_detailsButton_clicked()
{
    setShowDetails(!m_showDetails);
}
