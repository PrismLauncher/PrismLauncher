// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Prism Launcher Contributors
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

#include "DevelopingModPage.h"
#include "ui_DevelopingModPage.h"

#include <QDir>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QListWidgetItem>

#include "FileSystem.h"
#include "minecraft/mod/DevelopingModWatcher.h"

DevelopingModPage::DevelopingModPage(MinecraftInstance* inst, QWidget* parent)
    : QWidget(parent), ui(new Ui::DevelopingModPage), m_inst(inst)
{
    ui->setupUi(this);

    connect(ui->enableGroup, &QGroupBox::toggled, this, &DevelopingModPage::enableToggled);
    connect(ui->addFolderButton, &QPushButton::clicked, this, &DevelopingModPage::addFolder);
    connect(ui->removeFolderButton, &QPushButton::clicked, this, &DevelopingModPage::removeSelectedFolder);
    connect(ui->syncNowButton, &QPushButton::clicked, this, &DevelopingModPage::syncNowClicked);

    loadFromSettings();

    if (auto* watcher = m_inst->developingModWatcher()) {
        connect(watcher, &DevelopingModWatcher::statusChanged, this, &DevelopingModPage::updateStatusLabel);
        updateStatusLabel(watcher->statusText());
    }
}

DevelopingModPage::~DevelopingModPage()
{
    delete ui;
}

void DevelopingModPage::openedImpl()
{
    loadFromSettings();
    if (auto* watcher = m_inst->developingModWatcher())
        updateStatusLabel(watcher->statusText());
}

void DevelopingModPage::loadFromSettings()
{
    auto* settings = m_inst->settings();

    ui->enableGroup->blockSignals(true);
    ui->ignoreEdit->blockSignals(true);

    ui->enableGroup->setChecked(settings->get("DevelopingModEnabled").toBool());

    ui->foldersList->clear();
    const auto foldersRaw = settings->get("DevelopingModFolders").toString().trimmed();
    QJsonParseError error{};
    const auto doc = QJsonDocument::fromJson(foldersRaw.toUtf8(), &error);
    if (error.error == QJsonParseError::NoError && doc.isArray()) {
        for (const auto& value : doc.array()) {
            const auto path = value.toString().trimmed();
            if (!path.isEmpty())
                ui->foldersList->addItem(path);
        }
    }

    auto ignore = settings->get("DevelopingModIgnorePatterns").toString();
    if (ignore.trimmed().isEmpty())
        ignore = DevelopingModWatcher::defaultIgnorePatterns().join(QLatin1Char('\n'));
    ui->ignoreEdit->setPlainText(ignore);

    ui->enableGroup->blockSignals(false);
    ui->ignoreEdit->blockSignals(false);

    const bool enabled = ui->enableGroup->isChecked();
    ui->foldersList->setEnabled(enabled);
    ui->addFolderButton->setEnabled(enabled);
    ui->removeFolderButton->setEnabled(enabled);
    ui->ignoreEdit->setEnabled(enabled);
    ui->syncNowButton->setEnabled(enabled);
}

QStringList DevelopingModPage::foldersFromUi() const
{
    QStringList folders;
    for (int i = 0; i < ui->foldersList->count(); ++i) {
        const auto path = ui->foldersList->item(i)->text().trimmed();
        if (!path.isEmpty())
            folders << FS::NormalizePath(path);
    }
    folders.removeDuplicates();
    return folders;
}

void DevelopingModPage::saveFolders()
{
    QJsonArray array;
    for (const auto& folder : foldersFromUi())
        array.append(folder);
    m_inst->settings()->set("DevelopingModFolders", QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact)));
}

void DevelopingModPage::saveIgnorePatterns()
{
    m_inst->settings()->set("DevelopingModIgnorePatterns", ui->ignoreEdit->toPlainText());
}

void DevelopingModPage::enableToggled(bool checked)
{
    m_inst->settings()->set("DevelopingModEnabled", checked);
    ui->foldersList->setEnabled(checked);
    ui->addFolderButton->setEnabled(checked);
    ui->removeFolderButton->setEnabled(checked);
    ui->ignoreEdit->setEnabled(checked);
    ui->syncNowButton->setEnabled(checked);
}

void DevelopingModPage::addFolder()
{
    const auto path = QFileDialog::getExistingDirectory(this, tr("Select build output folder"), QDir::homePath());
    if (path.isEmpty())
        return;

    const auto normalized = FS::NormalizePath(path);
    for (int i = 0; i < ui->foldersList->count(); ++i) {
        if (FS::NormalizePath(ui->foldersList->item(i)->text()) == normalized)
            return;
    }

    ui->foldersList->addItem(normalized);
    saveFolders();
}

void DevelopingModPage::removeSelectedFolder()
{
    const auto items = ui->foldersList->selectedItems();
    if (items.isEmpty())
        return;

    for (auto* item : items)
        delete item;
    saveFolders();
}

void DevelopingModPage::syncNowClicked()
{
    apply();
    if (auto* watcher = m_inst->developingModWatcher())
        watcher->syncNow();
}

void DevelopingModPage::updateStatusLabel(const QString& status)
{
    ui->statusLabel->setText(tr("Status: %1").arg(status));
}

bool DevelopingModPage::apply()
{
    m_inst->settings()->set("DevelopingModEnabled", ui->enableGroup->isChecked());
    saveFolders();
    saveIgnorePatterns();
    return true;
}

void DevelopingModPage::retranslate()
{
    ui->retranslateUi(this);
}
