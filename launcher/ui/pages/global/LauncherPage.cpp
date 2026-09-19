// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (c) 2022 dada513 <dada513@protonmail.com>
 *  Copyright (C) 2022 Tayou <git@tayou.org>
 *  Copyright (C) 2024 TheKodeToad <TheKodeToad@proton.me>
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

#include "LauncherPage.h"
#include "config/GlobalConfig.h"
#include "ui_LauncherPage.h"

#include <QDir>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QTextCharFormat>

#include <FileSystem.h>
#include "Application.h"
#include "BuildConfig.h"
#include "DesktopServices.h"
#include "ui/themes/ITheme.h"
#include "ui/themes/ThemeManager.h"
#include "updater/ExternalUpdater.h"

#include <QApplication>
#include <QProcess>

// FIXME: possibly move elsewhere
enum InstSortMode {
    // Sort alphabetically by name.
    Sort_Name,
    // Sort by which instance was launched most recently.
    Sort_LastLaunch,
    // Sort by which instance has the most playtime.
    Sort_Playtime,
};

LauncherPage::LauncherPage(QWidget* parent) : QWidget(parent), ui(new Ui::LauncherPage)
{
    ui->setupUi(this);

    ui->sortingModeGroup->setId(ui->sortByNameBtn, Sort_Name);
    ui->sortingModeGroup->setId(ui->sortLastLaunchedBtn, Sort_LastLaunch);
    ui->sortingModeGroup->setId(ui->sortByPlaytimeBtn, Sort_Playtime);

    loadSettings();

    ui->updateSettingsBox->setHidden(!APPLICATION->updater());
}

LauncherPage::~LauncherPage()
{
    delete ui;
}

bool LauncherPage::apply()
{
    applySettings();
    return true;
}

bool LauncherPage::confirmInstanceDirPath(const QString& rawDir, const QString& cookedDir)
{
    if (FS::checkProblemticPathJava(QDir(cookedDir))) {
        QMessageBox warning;
        warning.setText(
            tr("You're trying to specify an instance folder which\'s path "
               "contains at least one \'!\'. "
               "Java is known to cause problems if that is the case, your "
               "instances (probably) won't start!"));
        warning.setInformativeText(
            tr("Do you really want to use this path? "
               "Selecting \"No\" will close this and not alter your instance path."));
        warning.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        if (warning.exec() != QMessageBox::Ok)
            return false;
    } else if (DesktopServices::isFlatpak() && rawDir.startsWith("/run/user")) {
        QMessageBox warning;
        warning.setText(tr("You're trying to specify an instance folder "
                           "which was granted temporarily via Flatpak.\n"
                           "This is known to cause problems. "
                           "After a restart the launcher might break, "
                           "because it will no longer have access to that directory.\n\n"
                           "Granting %1 access to it via Flatseal is recommended.")
                            .arg(BuildConfig.LAUNCHER_DISPLAYNAME));
        warning.setInformativeText(tr("Do you want to proceed anyway?"));
        warning.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        if (warning.exec() != QMessageBox::Ok)
            return false;
    }
    return true;
}

void LauncherPage::on_instDirBrowseBtn_clicked()
{
    QString rawDir = QFileDialog::getExistingDirectory(this, tr("Instance Folder"), ui->instDirTextBox->text());
    if (!rawDir.isEmpty() && QDir(rawDir).exists()) {
        QString cookedDir = FS::NormalizePath(rawDir);
        if (confirmInstanceDirPath(rawDir, cookedDir))
            ui->instDirTextBox->setText(cookedDir);
    }
}

void LauncherPage::on_addInstDirBtn_clicked()
{
    QString rawDir = QFileDialog::getExistingDirectory(this, tr("Additional Instance Folder"));
    if (rawDir.isEmpty() || !QDir(rawDir).exists())
        return;

    QString cookedDir = FS::NormalizePath(rawDir);
    if (cookedDir == FS::NormalizePath(ui->instDirTextBox->text())) {
        QMessageBox::warning(this, tr("Duplicate directory"), tr("This is already your primary instance directory."));
        return;
    }

    if (!ui->additionalInstDirsList->findItems(cookedDir, Qt::MatchFixedString).isEmpty()) {
        QMessageBox::warning(this, tr("Duplicate directory"), tr("This directory has already been added."));
        return;
    }

    if (!confirmInstanceDirPath(rawDir, cookedDir))
        return;

    ui->additionalInstDirsList->addItem(cookedDir);
}

void LauncherPage::on_removeInstDirBtn_clicked()
{
    qDeleteAll(ui->additionalInstDirsList->selectedItems());
}

void LauncherPage::on_iconsDirBrowseBtn_clicked()
{
    QString rawDir = QFileDialog::getExistingDirectory(this, tr("Icons Folder"), ui->iconsDirTextBox->text());

    // do not allow current dir - it's dirty. Do not allow dirs that don't exist
    if (!rawDir.isEmpty() && QDir(rawDir).exists()) {
        QString cookedDir = FS::NormalizePath(rawDir);
        ui->iconsDirTextBox->setText(cookedDir);
    }
}

void LauncherPage::on_modsDirBrowseBtn_clicked()
{
    QString rawDir = QFileDialog::getExistingDirectory(this, tr("Mods Folder"), ui->modsDirTextBox->text());

    // do not allow current dir - it's dirty. Do not allow dirs that don't exist
    if (!rawDir.isEmpty() && QDir(rawDir).exists()) {
        QString cookedDir = FS::NormalizePath(rawDir);
        ui->modsDirTextBox->setText(cookedDir);
    }
}

void LauncherPage::on_downloadsDirBrowseBtn_clicked()
{
    QString rawDir = QFileDialog::getExistingDirectory(this, tr("Downloads Folder"), ui->downloadsDirTextBox->text());

    if (!rawDir.isEmpty() && QDir(rawDir).exists()) {
        QString cookedDir = FS::NormalizePath(rawDir);
        ui->downloadsDirTextBox->setText(cookedDir);
    }
}

void LauncherPage::on_javaDirBrowseBtn_clicked()
{
    QString rawDir = QFileDialog::getExistingDirectory(this, tr("Java Folder"), ui->javaDirTextBox->text());

    if (!rawDir.isEmpty() && QDir(rawDir).exists()) {
        QString cookedDir = FS::NormalizePath(rawDir);
        ui->javaDirTextBox->setText(cookedDir);
    }
}

void LauncherPage::on_skinsDirBrowseBtn_clicked()
{
    QString rawDir = QFileDialog::getExistingDirectory(this, tr("Skins Folder"), ui->skinsDirTextBox->text());

    // do not allow current dir - it's dirty. Do not allow dirs that don't exist
    if (!rawDir.isEmpty() && QDir(rawDir).exists()) {
        QString cookedDir = FS::NormalizePath(rawDir);
        ui->skinsDirTextBox->setText(cookedDir);
    }
}

void LauncherPage::on_metadataEnableBtn_clicked()
{
    ui->metadataWarningLabel->setHidden(ui->metadataEnableBtn->isChecked());
}

void LauncherPage::applySettings()
{
    auto& conf = APPLICATION->config().update();

    // Updates
    if (APPLICATION->updater()) {
        APPLICATION->updater()->setAutomaticallyChecksForUpdates(ui->autoUpdateCheckBox->isChecked());
        APPLICATION->updater()->setUpdateCheckInterval(ui->updateIntervalSpinBox->value() * 3600);
    }

    conf.menuBarInsteadOfToolBar = ui->preferMenuBarCheckBox->isChecked();

    conf.numberOfConcurrentTasks = ui->numberOfConcurrentTasksSpinBox->value();
    conf.numberOfConcurrentDownloads = ui->numberOfConcurrentDownloadsSpinBox->value();
    conf.numberOfManualRetries = ui->numberOfManualRetriesSpinBox->value();
    conf.requestTimeout = ui->timeoutSecondsSpinBox->value();

    // Console settings
    conf.consoleMaxLines = ui->lineLimitSpinBox->value();
    conf.consoleOverflowStop = ui->checkStopLogging->checkState() != Qt::Unchecked;

    // Folders
    // TODO: Offer to move instances to new instance folder.
    conf.instanceDir = ui->instDirTextBox->text();
    {
        QStringList additionalDirs;
        for (int i = 0; i < ui->additionalInstDirsList->count(); ++i) {
            additionalDirs << ui->additionalInstDirsList->item(i)->text();
        }
        conf.additionalInstanceDirs = additionalDirs;
    }
    conf.centralModsDir = ui->modsDirTextBox->text();
    conf.iconsDir = ui->iconsDirTextBox->text();
    conf.downloadsDir = ui->downloadsDirTextBox->text();
    conf.skinsDir = ui->skinsDirTextBox->text();
    conf.javaDir = ui->javaDirTextBox->text();
    conf.downloadsDirWatchRecursive = ui->downloadsDirWatchRecursiveCheckBox->isChecked();
    conf.moveModsFromDownloadsDir = ui->downloadsDirMoveCheckBox->isChecked();

    // Instance
    auto sortMode = (InstSortMode)ui->sortingModeGroup->checkedId();
    switch (sortMode) {
        case Sort_LastLaunch:
            conf.instSortMode = "LastLaunch";
            break;
        case Sort_Playtime:
            conf.instSortMode = "Playtime";
            break;
        case Sort_Name:
        default:
            conf.instSortMode = "Name";
            break;
    }

    if (ui->askToRenameDirBtn->isChecked()) {
        conf.instRenamingMode = "AskEverytime";
    } else if (ui->alwaysRenameDirBtn->isChecked()) {
        conf.instRenamingMode = "PhysicalDir";
    } else if (ui->neverRenameDirBtn->isChecked()) {
        conf.instRenamingMode = "MetadataOnly";
    }

    conf.editInstanceOnDoubleClick = ui->editInstanceOnDoubleClick->isChecked();

    // Mods
    conf.modMetadataDisabled = !ui->metadataEnableBtn->isChecked();
    conf.modDependenciesDisabled = !ui->dependenciesEnableBtn->isChecked();
    conf.showModIncompat = ui->showModIncompatCheckBox->isChecked();
    conf.skipModpackUpdatePrompt = !ui->modpackUpdatePromptBtn->isChecked();
    conf.downloadGameFilesDuringInstanceCreation = ui->downloadGameFilesBtn->isChecked();
}
void LauncherPage::loadSettings()
{
    const auto& conf = *APPLICATION->config();
    // Updates
    if (APPLICATION->updater()) {
        ui->autoUpdateCheckBox->setChecked(APPLICATION->updater()->getAutomaticallyChecksForUpdates());
        ui->updateIntervalSpinBox->setValue(APPLICATION->updater()->getUpdateCheckInterval() / 3600);
    }

    ui->preferMenuBarCheckBox->setChecked(conf.menuBarInsteadOfToolBar);

    ui->numberOfConcurrentTasksSpinBox->setValue(conf.numberOfConcurrentTasks);
    ui->numberOfConcurrentDownloadsSpinBox->setValue(conf.numberOfConcurrentDownloads);
    ui->numberOfManualRetriesSpinBox->setValue(conf.numberOfManualRetries);
    ui->timeoutSecondsSpinBox->setValue(conf.requestTimeout);

    // Console settings
    ui->lineLimitSpinBox->setValue(conf.consoleMaxLines);
    ui->checkStopLogging->setChecked(conf.consoleOverflowStop);

    // Folders
    ui->instDirTextBox->setText(conf.instanceDir);
    ui->additionalInstDirsList->clear();
    ui->additionalInstDirsList->addItems(conf.additionalInstanceDirs);
    ui->modsDirTextBox->setText(conf.centralModsDir);
    ui->iconsDirTextBox->setText(conf.iconsDir);
    ui->downloadsDirTextBox->setText(conf.downloadsDir);
    ui->skinsDirTextBox->setText(conf.skinsDir);
    ui->javaDirTextBox->setText(conf.javaDir);
    ui->downloadsDirWatchRecursiveCheckBox->setChecked(conf.downloadsDirWatchRecursive);
    ui->downloadsDirMoveCheckBox->setChecked(conf.moveModsFromDownloadsDir);

    // Instance
    QString sortMode = conf.instSortMode;
    if (sortMode == "LastLaunch") {
        ui->sortLastLaunchedBtn->setChecked(true);
    } else if (sortMode == "Playtime") {
        ui->sortByPlaytimeBtn->setChecked(true);
    } else {
        ui->sortByNameBtn->setChecked(true);
    }

    ui->editInstanceOnDoubleClick->setChecked(conf.editInstanceOnDoubleClick);

    QString renamingMode = conf.instRenamingMode;
    ui->askToRenameDirBtn->setChecked(renamingMode == "AskEverytime");
    ui->alwaysRenameDirBtn->setChecked(renamingMode == "PhysicalDir");
    ui->neverRenameDirBtn->setChecked(renamingMode == "MetadataOnly");

    // Mods
    ui->metadataEnableBtn->setChecked(!conf.modMetadataDisabled);
    ui->metadataWarningLabel->setHidden(ui->metadataEnableBtn->isChecked());
    ui->dependenciesEnableBtn->setChecked(!conf.modDependenciesDisabled);
    ui->showModIncompatCheckBox->setChecked(conf.showModIncompat);
    ui->modpackUpdatePromptBtn->setChecked(!conf.skipModpackUpdatePrompt);
    ui->downloadGameFilesBtn->setChecked(conf.downloadGameFilesDuringInstanceCreation);
}

void LauncherPage::retranslate()
{
    ui->retranslateUi(this);
}
