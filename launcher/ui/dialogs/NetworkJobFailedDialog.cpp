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

#include <QClipboard>
#include <QPushButton>
#include <QShortcut>
#include <utility>

#include "ui_NetworkJobFailedDialog.h"

NetworkJobFailedDialog::NetworkJobFailedDialog(const QString& jobName, const int attempts, const int requests, const int failed, QWidget* parent)
    : QDialog(parent), m_ui(new Ui::NetworkJobFailedDialog)
{
    m_ui->setupUi(this);
    m_ui->failLabel->setText(m_ui->failLabel->text().arg(jobName));
    if (failed == requests) {
        m_ui->requestCountLabel->setText(tr("All %1 requests have failed after %2 attempts").arg(failed).arg(attempts));
    } else if (failed < requests / 2) {
        m_ui->requestCountLabel->setText(
            tr("Out of %1 requests, %2 have failed after %3 attempts").arg(requests).arg(failed).arg(attempts));
    } else {
        m_ui->requestCountLabel->setText(
            tr("Out of %1 requests, only %2 succeeded after %3 attempts").arg(requests).arg(requests - failed).arg(attempts));
    }

    m_ui->detailsTable->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_ui->detailsTable->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    m_ui->detailsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);

    const auto* copyShortcut = new QShortcut(QKeySequence::Copy, m_ui->detailsTable);
    connect(copyShortcut, &QShortcut::activated, this, &NetworkJobFailedDialog::copyUrl);

    const auto* copyButton = m_ui->dialogButtonBox->addButton(tr("Copy URL"), QDialogButtonBox::ActionRole);
    connect(copyButton, &QPushButton::clicked, this, &NetworkJobFailedDialog::copyUrl);

    connect(m_ui->dialogButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_ui->dialogButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

NetworkJobFailedDialog::~NetworkJobFailedDialog()
{
    delete m_ui;
}

void NetworkJobFailedDialog::addFailedRequest(const QUrl& url, QString error) const
{
    auto* item = new QTreeWidgetItem(m_ui->detailsTable, { url.toString(), std::move(error) });
    m_ui->detailsTable->addTopLevelItem(item);
    if (m_ui->detailsTable->selectedItems().isEmpty()) {
        m_ui->detailsTable->setCurrentItem(item);
    }
}

void NetworkJobFailedDialog::copyUrl() const
{
    auto items = m_ui->detailsTable->selectedItems();
    if (items.isEmpty()) {
        return;
    }

    QString urls = items.first()->text(0);
    for (auto& item : items.sliced(1)) {
        urls += "\n" + item->text(0);
    }

    auto* clipboard = QGuiApplication::clipboard();
    clipboard->setText(urls);
}
