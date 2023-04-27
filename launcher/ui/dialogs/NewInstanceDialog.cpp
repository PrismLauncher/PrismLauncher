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

#include "Application.h"
#include "NewInstanceDialog.h"
#include "ui_NewInstanceDialog.h"

#include <BaseVersion.h>
#include <icons/IconList.h>
#include <tasks/Task.h>
#include <InstanceList.h>

#include "VersionSelectDialog.h"
#include "ProgressDialog.h"
#include "IconPickerDialog.h"

#include <QLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QValidator>
#include <QDialogButtonBox>
#include <utility>

#include "ui/widgets/PageContainer.h"
#include "ui/pages/modplatform/VanillaPage.h"
#include "ui/pages/modplatform/atlauncher/AtlPage.h"
#include "ui/pages/modplatform/legacy_ftb/Page.h"
#include "ui/pages/modplatform/flame/FlamePage.h"
#include "ui/pages/modplatform/ImportPage.h"
#include "ui/pages/modplatform/modrinth/ModrinthPage.h"
#include "ui/pages/modplatform/technic/TechnicPage.h"



NewInstanceDialog::NewInstanceDialog(const QString & initialGroup, const QString & url, QWidget *parent)
    : QDialog(parent), ui(new Ui::NewInstanceDialog)
{
    ui->setupUi(this);

    setWindowIcon(APPLICATION->getThemedIcon("new"));

    InstIconKey = "default";
    ui->iconButton->setIcon(APPLICATION->icons()->getIcon(InstIconKey));

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    auto groupList = APPLICATION->instances()->getGroups();
    auto groups = QSet<QString>(groupList.begin(), groupList.end());
    groupList = groups.values();
#else
    auto groups = APPLICATION->instances()->getGroups().toSet();
    auto groupList = QStringList(groups.toList());
#endif
    groupList.sort(Qt::CaseInsensitive);
    groupList.removeOne("");
    groupList.push_front(initialGroup);
    groupList.push_front("");
    ui->groupBox->addItems(groupList);
    int index = groupList.indexOf(initialGroup);
    if(index == -1)
    {
        index = 0;
    }
    ui->groupBox->setCurrentIndex(index);
    ui->groupBox->lineEdit()->setPlaceholderText(tr("No group"));


    // NOTE: m_buttons must be initialized before PageContainer, because it indirectly accesses m_buttons through setSuggestedPack! Do not move this below.
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Help | QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    m_container = new PageContainer(this);
    m_container->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Expanding);
    m_container->layout()->setContentsMargins(0, 0, 0, 0);
    ui->verticalLayout->insertWidget(2, m_container);

    m_container->addButtons(m_buttons);

    // Bonk Qt over its stupid head and make sure it understands which button is the default one...
    // See: https://stackoverflow.com/questions/24556831/qbuttonbox-set-default-button
    auto OkButton = m_buttons->button(QDialogButtonBox::Ok);
    OkButton->setDefault(true);
    OkButton->setAutoDefault(true);
    connect(OkButton, &QPushButton::clicked, this, &NewInstanceDialog::accept);

    auto CancelButton = m_buttons->button(QDialogButtonBox::Cancel);
    CancelButton->setDefault(false);
    CancelButton->setAutoDefault(false);
    connect(CancelButton, &QPushButton::clicked, this, &NewInstanceDialog::reject);

    auto HelpButton = m_buttons->button(QDialogButtonBox::Help);
    HelpButton->setDefault(false);
    HelpButton->setAutoDefault(false);
    connect(HelpButton, &QPushButton::clicked, m_container, &PageContainer::help);

    if(!url.isEmpty())
    {
        QUrl actualUrl(url);
        m_container->selectPage("import");
        importPage->setUrl(url);
    }

    updateDialogState();

    restoreGeometry(QByteArray::fromBase64(APPLICATION->settings()->get("NewInstanceGeometry").toByteArray()));
}

void NewInstanceDialog::reject()
{
    APPLICATION->settings()->set("NewInstanceGeometry", saveGeometry().toBase64());

    // This is just so that the pages get the close() call and can react to it, if needed.
    m_container->prepareToClose();

    QDialog::reject();
}

void NewInstanceDialog::accept()
{
    APPLICATION->settings()->set("NewInstanceGeometry", saveGeometry().toBase64());
    importIconNow();

    // This is just so that the pages get the close() call and can react to it, if needed.
    m_container->prepareToClose();

    QDialog::accept();
}

QList<BasePage *> NewInstanceDialog::getPages()
{
    QList<BasePage *> pages;

    importPage = new ImportPage(this);

    pages.append(new VanillaPage(this));
    pages.append(importPage);
    pages.append(new AtlPage(this));
    if (APPLICATION->capabilities() & Application::SupportsFlame)
        pages.append(new FlamePage(this));
    pages.append(new LegacyFTB::Page(this));
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

void NewInstanceDialog::setSuggestedPack(const QString& name, InstanceTask* task)
{
    creationTask.reset(task);

    ui->instNameTextBox->setPlaceholderText(name);
    importVersion.clear();

    if (!task) {
        ui->iconButton->setIcon(APPLICATION->icons()->getIcon("default"));
        importIcon = false;
    }

    auto allowOK = task && !instName().isEmpty();
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(allowOK);
}

void NewInstanceDialog::setSuggestedPack(const QString& name, QString version, InstanceTask* task)
{
    creationTask.reset(task);

    ui->instNameTextBox->setPlaceholderText(name);
    importVersion = std::move(version);

    if (!task) {
        ui->iconButton->setIcon(APPLICATION->icons()->getIcon("default"));
        importIcon = false;
    }

    auto allowOK = task && !instName().isEmpty();
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(allowOK);
}

void NewInstanceDialog::setSuggestedIconFromFile(const QString &path, const QString &name)
{
    importIcon = true;
    importIconPath = path;
    importIconName = name;

    //Hmm, for some reason they can be to small
    ui->iconButton->setIcon(QIcon(path));
}

void NewInstanceDialog::setSuggestedIcon(const QString &key)
{
    auto icon = APPLICATION->icons()->getIcon(key);
    importIcon = false;

    ui->iconButton->setIcon(icon);
}

InstanceTask * NewInstanceDialog::extractTask()
{
    InstanceTask * extracted = creationTask.get();
    creationTask.release();

    InstanceName inst_name(ui->instNameTextBox->placeholderText().trimmed(), importVersion);
    inst_name.setName(ui->instNameTextBox->text().trimmed());
    extracted->setName(inst_name);

    extracted->setGroup(instGroup());
    extracted->setIcon(iconKey());
    return extracted;
}

void NewInstanceDialog::updateDialogState()
{
    auto allowOK = creationTask && !instName().isEmpty();
    auto OkButton = m_buttons->button(QDialogButtonBox::Ok);
    if(OkButton->isEnabled() != allowOK)
    {
        OkButton->setEnabled(allowOK);
    }
}

QString NewInstanceDialog::instName() const
{
    auto result = ui->instNameTextBox->text().trimmed();
    if(result.size())
    {
        return result;
    }
    result = ui->instNameTextBox->placeholderText().trimmed();
    if(result.size())
    {
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
    return InstIconKey;
}

void NewInstanceDialog::on_iconButton_clicked()
{
    importIconNow(); //so the user can switch back
    IconPickerDialog dlg(this);
    dlg.execWithSelection(InstIconKey);

    if (dlg.result() == QDialog::Accepted)
    {
        InstIconKey = dlg.selectedIconKey;
        ui->iconButton->setIcon(APPLICATION->icons()->getIcon(InstIconKey));
        importIcon = false;
    }
}

void NewInstanceDialog::on_instNameTextBox_textChanged(const QString &arg1)
{
    updateDialogState();
}

void NewInstanceDialog::importIconNow()
{
    if(importIcon) {
        APPLICATION->icons()->installIcon(importIconPath, importIconName);
        InstIconKey = importIconName;
        importIcon = false;
    }
    APPLICATION->settings()->set("NewInstanceGeometry", saveGeometry().toBase64());
}
