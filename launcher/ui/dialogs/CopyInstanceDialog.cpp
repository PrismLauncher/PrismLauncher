// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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

#include <QLayout>
#include <QPushButton>

#include "Application.h"
#include "BuildConfig.h"
#include "CopyInstanceDialog.h"
#include "ui_CopyInstanceDialog.h"

#include "ui/dialogs/IconPickerDialog.h"

#include "DesktopServices.h"
#include "FileSystem.h"
#include "InstanceList.h"
#include "icons/IconList.h"
#include "minecraft/MinecraftInstance.h"

CopyInstanceDialog::CopyInstanceDialog(MinecraftInstance* original, QWidget* parent)
    : QDialog(parent), m_ui(new Ui::CopyInstanceDialog), m_original(original)
{
    m_ui->setupUi(this);
    resize(minimumSizeHint());
    layout()->setSizeConstraint(QLayout::SetFixedSize);

    m_instIconKey = original->iconKey();
    m_ui->iconButton->setIcon(APPLICATION->icons()->getIcon(m_instIconKey));
    m_ui->instNameTextBox->setText(original->name());
    m_ui->instNameTextBox->setFocus();

    QStringList groups = APPLICATION->instances()->getGroups();
    groups.prepend("");
    m_ui->groupBox->addItems(groups);
    int index = groups.indexOf(APPLICATION->instances()->getInstanceGroup(m_original->id()));
    if (index == -1) {
        index = 0;
    }

    m_ui->groupBox->setCurrentIndex(index);
    m_ui->groupBox->lineEdit()->setPlaceholderText(tr("No group"));
    m_ui->copySavesCheckbox->setChecked(m_selectedOptions.isCopySavesEnabled());
    m_ui->keepPlaytimeCheckbox->setChecked(m_selectedOptions.isKeepPlaytimeEnabled());
    m_ui->copyGameOptionsCheckbox->setChecked(m_selectedOptions.isCopyGameOptionsEnabled());
    m_ui->copyResPacksCheckbox->setChecked(m_selectedOptions.isCopyResourcePacksEnabled());
    m_ui->copyShaderPacksCheckbox->setChecked(m_selectedOptions.isCopyShaderPacksEnabled());
    m_ui->copyServersCheckbox->setChecked(m_selectedOptions.isCopyServersEnabled());
    m_ui->copyModsCheckbox->setChecked(m_selectedOptions.isCopyModsEnabled());
    m_ui->copyScreenshotsCheckbox->setChecked(m_selectedOptions.isCopyScreenshotsEnabled());

    m_ui->symbolicLinksCheckbox->setChecked(m_selectedOptions.isUseSymLinksEnabled());
    m_ui->hardLinksCheckbox->setChecked(m_selectedOptions.isUseHardLinksEnabled());

    m_ui->recursiveLinkCheckbox->setChecked(m_selectedOptions.isLinkRecursivelyEnabled());
    m_ui->dontLinkSavesCheckbox->setChecked(m_selectedOptions.isDontLinkSavesEnabled());

    auto detectedFS = FS::statFS(m_original->instanceRoot()).fsType;

    m_cloneSupported = FS::canCloneOnFS(detectedFS);
    m_linkSupported = FS::canLinkOnFS(detectedFS);

    if (m_cloneSupported) {
        m_ui->cloneSupportedLabel->setText(tr("Reflinks are supported on %1").arg(FS::getFilesystemTypeName(detectedFS)));
    } else {
        m_ui->cloneSupportedLabel->setText(tr("Reflinks aren't supported on %1").arg(FS::getFilesystemTypeName(detectedFS)));
    }

#if defined(Q_OS_WIN)
    m_ui->symbolicLinksCheckbox->setIcon(style()->standardIcon(QStyle::SP_VistaShield));
    m_ui->symbolicLinksCheckbox->setToolTip(tr("Use symbolic links instead of copying files.") + "\n" +
                                            tr("On Windows, symbolic links may require admin permission to create."));
#endif

    updateLinkOptions();
    updateUseCloneCheckbox();

    auto* helpButton = m_ui->buttonBox->button(QDialogButtonBox::Help);
    connect(helpButton, &QPushButton::clicked, this, &CopyInstanceDialog::help);
    helpButton->setText(tr("Help"));
    m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));
}

CopyInstanceDialog::~CopyInstanceDialog()
{
    delete m_ui;
}

void CopyInstanceDialog::updateDialogState()
{
    auto allowOK = !instName().isEmpty();
    auto* okButton = m_ui->buttonBox->button(QDialogButtonBox::Ok);
    if (okButton->isEnabled() != allowOK) {
        okButton->setEnabled(allowOK);
    }
}

QString CopyInstanceDialog::instName() const
{
    auto result = m_ui->instNameTextBox->text().trimmed();
    if (result.size() != 0) {
        return result;
    }
    return QString();
}

QString CopyInstanceDialog::iconKey() const
{
    return m_instIconKey;
}

QString CopyInstanceDialog::instGroup() const
{
    return m_ui->groupBox->currentText();
}

const InstanceCopyPrefs& CopyInstanceDialog::getChosenOptions() const
{
    return m_selectedOptions;
}

void CopyInstanceDialog::help()
{
    DesktopServices::openUrl(QUrl(BuildConfig.HELP_URL.arg("instance-copy")));
}

void CopyInstanceDialog::checkAllCheckboxes(const bool& b)
{
    m_ui->keepPlaytimeCheckbox->setChecked(b);
    m_ui->copySavesCheckbox->setChecked(b);
    m_ui->copyGameOptionsCheckbox->setChecked(b);
    m_ui->copyResPacksCheckbox->setChecked(b);
    m_ui->copyShaderPacksCheckbox->setChecked(b);
    m_ui->copyServersCheckbox->setChecked(b);
    m_ui->copyModsCheckbox->setChecked(b);
    m_ui->copyScreenshotsCheckbox->setChecked(b);
}

// Check the "Select all" checkbox if all options are already selected:
void CopyInstanceDialog::updateSelectAllCheckbox()
{
    m_ui->selectAllCheckbox->blockSignals(true);
    m_ui->selectAllCheckbox->setChecked(m_selectedOptions.allTrue());
    m_ui->selectAllCheckbox->blockSignals(false);
}

void CopyInstanceDialog::updateUseCloneCheckbox()
{
    m_ui->useCloneCheckbox->setEnabled(m_cloneSupported && !m_ui->symbolicLinksCheckbox->isChecked() &&
                                       !m_ui->hardLinksCheckbox->isChecked());
    m_ui->useCloneCheckbox->setChecked(m_cloneSupported && m_selectedOptions.isUseCloneEnabled() &&
                                       !m_ui->symbolicLinksCheckbox->isChecked() && !m_ui->hardLinksCheckbox->isChecked());
}

void CopyInstanceDialog::updateLinkOptions()
{
    m_ui->symbolicLinksCheckbox->setEnabled(m_linkSupported && !m_ui->hardLinksCheckbox->isChecked() &&
                                            !m_ui->useCloneCheckbox->isChecked());
    m_ui->hardLinksCheckbox->setEnabled(m_linkSupported && !m_ui->symbolicLinksCheckbox->isChecked() &&
                                        !m_ui->useCloneCheckbox->isChecked());

    m_ui->symbolicLinksCheckbox->setChecked(m_linkSupported && m_selectedOptions.isUseSymLinksEnabled() &&
                                            !m_ui->useCloneCheckbox->isChecked());
    m_ui->hardLinksCheckbox->setChecked(m_linkSupported && m_selectedOptions.isUseHardLinksEnabled() &&
                                        !m_ui->useCloneCheckbox->isChecked());

    bool linksInUse = (m_ui->symbolicLinksCheckbox->isChecked() || m_ui->hardLinksCheckbox->isChecked());
    m_ui->recursiveLinkCheckbox->setEnabled(m_linkSupported && linksInUse && !m_ui->hardLinksCheckbox->isChecked());
    m_ui->dontLinkSavesCheckbox->setEnabled(m_linkSupported && linksInUse);
    m_ui->recursiveLinkCheckbox->setChecked(m_linkSupported && linksInUse && m_selectedOptions.isLinkRecursivelyEnabled());
    m_ui->dontLinkSavesCheckbox->setChecked(m_linkSupported && linksInUse && m_selectedOptions.isDontLinkSavesEnabled());

#if defined(Q_OS_WIN)
    auto OkButton = m_ui->buttonBox->button(QDialogButtonBox::Ok);
    OkButton->setIcon(m_selectedOptions.isUseSymLinksEnabled() ? style()->standardIcon(QStyle::SP_VistaShield) : QIcon());
#endif
}

void CopyInstanceDialog::on_iconButton_clicked()
{
    IconPickerDialog dlg(this);
    dlg.execWithSelection(m_instIconKey);

    if (dlg.result() == QDialog::Accepted) {
        m_instIconKey = dlg.selectedIconKey;
        m_ui->iconButton->setIcon(APPLICATION->icons()->getIcon(m_instIconKey));
    }
}

void CopyInstanceDialog::on_instNameTextBox_textChanged([[maybe_unused]] const QString& arg1)
{
    updateDialogState();
}

void CopyInstanceDialog::on_selectAllCheckbox_stateChanged(int state)
{
    bool checked = false;
    checked = (state == Qt::Checked);
    checkAllCheckboxes(checked);
}

void CopyInstanceDialog::on_copySavesCheckbox_stateChanged(int state)
{
    m_selectedOptions.enableCopySaves(state == Qt::Checked);
    m_ui->dontLinkSavesCheckbox->setChecked((state == Qt::Checked) && m_ui->dontLinkSavesCheckbox->isChecked());
    updateSelectAllCheckbox();
}

void CopyInstanceDialog::on_keepPlaytimeCheckbox_stateChanged(int state)
{
    m_selectedOptions.enableKeepPlaytime(state == Qt::Checked);
    updateSelectAllCheckbox();
}

void CopyInstanceDialog::on_copyGameOptionsCheckbox_stateChanged(int state)
{
    m_selectedOptions.enableCopyGameOptions(state == Qt::Checked);
    updateSelectAllCheckbox();
}

void CopyInstanceDialog::on_copyResPacksCheckbox_stateChanged(int state)
{
    m_selectedOptions.enableCopyResourcePacks(state == Qt::Checked);
    updateSelectAllCheckbox();
}

void CopyInstanceDialog::on_copyShaderPacksCheckbox_stateChanged(int state)
{
    m_selectedOptions.enableCopyShaderPacks(state == Qt::Checked);
    updateSelectAllCheckbox();
}

void CopyInstanceDialog::on_copyServersCheckbox_stateChanged(int state)
{
    m_selectedOptions.enableCopyServers(state == Qt::Checked);
    updateSelectAllCheckbox();
}

void CopyInstanceDialog::on_copyModsCheckbox_stateChanged(int state)
{
    m_selectedOptions.enableCopyMods(state == Qt::Checked);
    updateSelectAllCheckbox();
}

void CopyInstanceDialog::on_copyScreenshotsCheckbox_stateChanged(int state)
{
    m_selectedOptions.enableCopyScreenshots(state == Qt::Checked);
    updateSelectAllCheckbox();
}

void CopyInstanceDialog::on_symbolicLinksCheckbox_stateChanged(int state)
{
    m_selectedOptions.enableUseSymLinks(state == Qt::Checked);
    updateUseCloneCheckbox();
    updateLinkOptions();
}

void CopyInstanceDialog::on_hardLinksCheckbox_stateChanged(int state)
{
    m_selectedOptions.enableUseHardLinks(state == Qt::Checked);
    if (state == Qt::Checked && !m_ui->recursiveLinkCheckbox->isChecked()) {
        m_ui->recursiveLinkCheckbox->setChecked(true);
    }
    updateUseCloneCheckbox();
    updateLinkOptions();
}

void CopyInstanceDialog::on_recursiveLinkCheckbox_stateChanged(int state)
{
    m_selectedOptions.enableLinkRecursively(state == Qt::Checked);
    updateLinkOptions();
}

void CopyInstanceDialog::on_dontLinkSavesCheckbox_stateChanged(int state)
{
    m_selectedOptions.enableDontLinkSaves(state == Qt::Checked);
}

void CopyInstanceDialog::on_useCloneCheckbox_stateChanged(int state)
{
    m_selectedOptions.enableUseClone(m_cloneSupported && (state == Qt::Checked));
    updateUseCloneCheckbox();
    updateLinkOptions();
}
