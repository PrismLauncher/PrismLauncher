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

#include "NewInstanceDialog.h"
#include "Application.h"
#include "ui/pages/modplatform/ModpackProviderBasePage.h"
#include "ui/pages/modplatform/import_ftb/ImportFTBPage.h"
#include "ui_NewInstanceDialog.h"

#include <BaseVersion.h>
#include <InstanceList.h>
#include <icons/IconList.h>
#include <tasks/Task.h>

#include "IconPickerDialog.h"
#include "VersionSelectDialog.h"
#include "ui/dialogs/CustomMessageBox.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QLayout>
#include <QPushButton>
#include <QScreen>
#include <QTimer>
#include <QValidator>
#include <utility>

#include "ui/pages/modplatform/CustomPage.h"
#include "ui/pages/modplatform/ImportPage.h"
#include "ui/pages/modplatform/atlauncher/AtlPage.h"
#include "ui/pages/modplatform/flame/FlamePage.h"
#include "ui/pages/modplatform/ftb/FtbPage.h"
#include "ui/pages/modplatform/legacy_ftb/Page.h"
#include "ui/pages/modplatform/modrinth/ModrinthPage.h"
#include "ui/pages/modplatform/technic/TechnicPage.h"
#include "ui/widgets/PageContainer.h"

NewInstanceDialog::NewInstanceDialog(const QString& initialGroup,
                                     const QString& url,
                                     const QMap<QString, QString>& extraInfo,
                                     QWidget* parent)
    : QDialog(parent), ui(new Ui::NewInstanceDialog), m_instIconKey("default")
{
    ui->setupUi(this);

    ui->instNameTextBox->installEventFilter(this);

    refreshInstDirBox();

    auto lastUsedDir = APPLICATION->settings()->get("LastUsedInstDirForNewInstance").toString();
    int lastUsedIdx = ui->instDirBox->findData(lastUsedDir);
    ui->instDirBox->setCurrentIndex(lastUsedIdx >= 0 ? lastUsedIdx : 0);

    setWindowIcon(QIcon::fromTheme("new"));

    ui->iconButton->setIcon(APPLICATION->icons()->getIcon(m_instIconKey));

    QStringList groups = APPLICATION->instances()->getGroups();
    groups.prepend("");
    auto index = groups.indexOf(initialGroup);
    if (index == -1) {
        index = 1;
        groups.insert(index, initialGroup);
    }
    ui->groupBox->addItems(groups);
    ui->groupBox->setCurrentIndex(index);
    ui->groupBox->lineEdit()->setPlaceholderText(tr("No group"));

    // NOTE: m_buttons must be initialized before PageContainer, because it indirectly accesses m_buttons through setSuggestedPack! Do not
    // move this below.
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Help | QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    m_container = new PageContainer(this, {}, this);
    m_container->useSidebarStyle(false);
    m_container->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Expanding);
    m_container->layout()->setContentsMargins(0, 0, 0, 0);
    ui->verticalLayout->insertWidget(2, m_container);

    m_container->addButtons(m_buttons);
    connect(m_container, &PageContainer::selectedPageChanged, this, [this](BasePage* /*previous*/, BasePage* /*selected*/) {
        m_buttons->button(QDialogButtonBox::Ok)->setEnabled(m_creationTask && !instName().isEmpty());
    });

    // Bonk Qt over its stupid head and make sure it understands which button is the default one...
    // See: https://stackoverflow.com/questions/24556831/qbuttonbox-set-default-button
    auto* okButton = m_buttons->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setAutoDefault(true);
    okButton->setText(tr("OK"));
    connect(okButton, &QPushButton::clicked, this, &NewInstanceDialog::accept);

    auto* cancelButton = m_buttons->button(QDialogButtonBox::Cancel);
    cancelButton->setDefault(false);
    cancelButton->setAutoDefault(false);
    cancelButton->setText(tr("Cancel"));
    connect(cancelButton, &QPushButton::clicked, this, &NewInstanceDialog::reject);

    auto* helpButton = m_buttons->button(QDialogButtonBox::Help);
    helpButton->setDefault(false);
    helpButton->setAutoDefault(false);
    helpButton->setText(tr("Help"));
    connect(helpButton, &QPushButton::clicked, m_container, &PageContainer::help);

    if (!url.isEmpty()) {
        QUrl actualUrl(url);
        m_container->selectPage("import");
        m_importPage->setUrl(url);
        m_importPage->setExtraInfo(extraInfo);
    }

    updateDialogState();

    if (APPLICATION->settings()->get("NewInstanceGeometry").isValid()) {
        restoreGeometry(QByteArray::fromBase64(APPLICATION->settings()->get("NewInstanceGeometry").toString().toUtf8()));
    } else {
        auto* screen = parent->screen();
        auto geometry = screen->availableSize();
        resize(width(), qMin(geometry.height() - 50, 710));
    }

    connect(m_container, &PageContainer::selectedPageChanged, this, &NewInstanceDialog::selectedPageChanged);
}

void NewInstanceDialog::reject()
{
    APPLICATION->settings()->set("NewInstanceGeometry", QString::fromUtf8(saveGeometry().toBase64()));

    // This is just so that the pages get the close() call and can react to it, if needed.
    m_container->prepareToClose();

    QDialog::reject();
}

void NewInstanceDialog::accept()
{
    auto chosenDir = instDir();
    if (!chosenDir.isEmpty() && !QDir(chosenDir).exists()) {
        CustomMessageBox::selectable(
            this, tr("Directory unavailable"),
            tr("The instance directory \"%1\" is no longer accessible. Please choose another location.").arg(chosenDir),
            QMessageBox::Warning)
            ->exec();
        refreshInstDirBox();
        return;
    }

    APPLICATION->settings()->set("NewInstanceGeometry", QString::fromUtf8(saveGeometry().toBase64()));
    importIconNow();

    // This is just so that the pages get the close() call and can react to it, if needed.
    m_container->prepareToClose();

    QDialog::accept();
}

QList<BasePage*> NewInstanceDialog::getPages()
{
    QList<BasePage*> pages;

    m_importPage = new ImportPage(this);

    pages.append(new CustomPage(this));
    pages.append(m_importPage);
    pages.append(new AtlPage(this));
    if (APPLICATION->capabilities() & Application::SupportsFlame) {
        pages.append(new FlamePage(this));
    }
    pages.append(new FtbPage(this));
    pages.append(new LegacyFTB::Page(this));
    pages.append(new FTBImportAPP::ImportFTBPage(this));
    pages.append(new ModrinthPage(this));
    pages.append(new TechnicPage(this));

    return pages;
}

QString NewInstanceDialog::dialogTitle()
{
    return tr("New Instance");
}

NewInstanceDialog::~NewInstanceDialog()
{
    delete ui;
}

void NewInstanceDialog::refreshInstDirBox()
{
    QString previouslySelected = ui->instDirBox->currentData().toString();
    ui->instDirBox->clear();

    auto addAccessibleDir = [this](const QString& dir, const QString& label) {
        if (dir.isEmpty())
            return;
        QString canonical = QDir(dir).canonicalPath();
        if (canonical.isEmpty())
            return;
        ui->instDirBox->addItem(label.isEmpty() ? canonical : label, canonical);
    };

    auto instDir = APPLICATION->settings()->get("InstanceDir").toString();
    addAccessibleDir(instDir, tr("Default (%1)").arg(instDir));
    for (const auto& dir : APPLICATION->settings()->get("AdditionalInstanceDirs").toStringList()) {
        addAccessibleDir(dir, {});
    }

    int idx = ui->instDirBox->findData(previouslySelected);
    ui->instDirBox->setCurrentIndex(idx >= 0 ? idx : 0);
}

void NewInstanceDialog::setSuggestedPack(const QString& name, InstanceTask* task)
{
    m_creationTask.reset(task);

    m_suggestedName = name;

    if (!m_nameFieldEditedByUser) {
        ui->instNameTextBox->blockSignals(true);
        ui->instNameTextBox->setText(name);
        updateDialogState();
        ui->instNameTextBox->blockSignals(false);
        m_nameFieldSelectedOnce = false;
    }
    m_importVersion.clear();

    if (!task) {
        ui->iconButton->setIcon(APPLICATION->icons()->getIcon(m_instIconKey));
        m_importIcon = false;
    }

    auto allowOK = task != nullptr && !instName().isEmpty();
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(allowOK);
}

void NewInstanceDialog::setSuggestedPack(const QString& name, QString version, InstanceTask* task)
{
    m_creationTask.reset(task);

    m_suggestedName = name;

    if (!m_nameFieldEditedByUser) {
        ui->instNameTextBox->blockSignals(true);
        ui->instNameTextBox->setText(name);
        updateDialogState();
        ui->instNameTextBox->blockSignals(false);
        m_nameFieldSelectedOnce = false;
    }
    m_importVersion = std::move(version);

    if (!task) {
        ui->iconButton->setIcon(APPLICATION->icons()->getIcon(m_instIconKey));
        m_importIcon = false;
    }

    auto allowOK = task != nullptr && !instName().isEmpty();
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(allowOK);
}

void NewInstanceDialog::setSuggestedIconFromFile(const QString& path, const QString& name)
{
    m_importIcon = true;
    m_importIconPath = path;
    m_importIconName = name;

    // Hmm, for some reason they can be to small
    ui->iconButton->setIcon(QIcon(path));
}

void NewInstanceDialog::setSuggestedIcon(const QString& key)
{
    if (key == "default") {
        return;
    }

    auto icon = APPLICATION->icons()->getIcon(key);
    m_importIcon = false;

    ui->iconButton->setIcon(icon);
}

InstanceTask* NewInstanceDialog::extractTask()
{
    InstanceTask* extracted = m_creationTask.release();

    extracted->setName(ui->instNameTextBox->text().trimmed());
    extracted->setOriginalName(m_suggestedName.trimmed(), m_importVersion);

    extracted->setGroup(instGroup());
    extracted->setIcon(iconKey());
    extracted->setTargetDir(ui->instDirBox->currentData().toString());
    return extracted;
}

void NewInstanceDialog::updateDialogState()
{
    auto allowOK = m_creationTask && !instName().isEmpty();
    auto* okButton = m_buttons->button(QDialogButtonBox::Ok);
    if (okButton->isEnabled() != allowOK) {
        okButton->setEnabled(allowOK);
    }
}

QString NewInstanceDialog::instName() const
{
    auto result = ui->instNameTextBox->text().trimmed();
    if (!result.isEmpty()) {
        return result;
    }
    result = m_suggestedName.trimmed();
    if (!result.isEmpty()) {
        return result;
    }
    return QString();
}

QString NewInstanceDialog::instGroup() const
{
    return ui->groupBox->currentText();
}
QString NewInstanceDialog::iconKey() const
{
    return m_instIconKey;
}

QString NewInstanceDialog::instDir() const
{
    return ui->instDirBox->currentData().toString();
}

void NewInstanceDialog::on_iconButton_clicked()
{
    importIconNow();  // so the user can switch back
    IconPickerDialog dlg(this);
    dlg.execWithSelection(m_instIconKey);

    if (dlg.result() == QDialog::Accepted) {
        m_instIconKey = dlg.selectedIconKey;
        ui->iconButton->setIcon(APPLICATION->icons()->getIcon(m_instIconKey));
        m_importIcon = false;
    }
}

void NewInstanceDialog::on_instNameTextBox_textChanged([[maybe_unused]] const QString& arg1)
{
    m_nameFieldEditedByUser = true;
    updateDialogState();
}

void NewInstanceDialog::importIconNow()
{
    if (m_importIcon) {
        APPLICATION->icons()->installIcon(m_importIconPath, m_importIconName);
        m_instIconKey = m_importIconName.mid(0, m_importIconName.lastIndexOf('.'));
        m_importIcon = false;
    }
    APPLICATION->settings()->set("NewInstanceGeometry", QString::fromUtf8(saveGeometry().toBase64()));
}

bool NewInstanceDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->instNameTextBox && event->type() == QEvent::FocusIn && !m_nameFieldSelectedOnce) {
        m_nameFieldSelectedOnce = true;
        QTimer::singleShot(0, ui->instNameTextBox, &QLineEdit::selectAll);
    }
    return QDialog::eventFilter(watched, event);
}

void NewInstanceDialog::selectedPageChanged(BasePage* previous, BasePage* selected)
{
    auto* prevPage = dynamic_cast<ModpackProviderBasePage*>(previous);
    if (prevPage) {
        m_searchTerm = prevPage->getSerachTerm();
    }

    auto* nextPage = dynamic_cast<ModpackProviderBasePage*>(selected);
    if (nextPage) {
        nextPage->setSearchTerm(m_searchTerm);
    }
}
