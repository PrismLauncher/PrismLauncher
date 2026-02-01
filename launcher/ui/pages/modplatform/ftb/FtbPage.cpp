// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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
 *      Copyright 2020-2021 Jamie Mansfield <jmansfield@cadixdev.org>
 *      Copyright 2021 Philip T <me@phit.link>
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

#include "FtbPage.h"
#include "ui_FtbPage.h"

#include <QKeyEvent>

#include "modplatform/ftb/FTBPackInstallTask.h"
#include "ui/dialogs/NewInstanceDialog.h"

#include "Markdown.h"

FtbPage::FtbPage(NewInstanceDialog* dialog, QWidget* parent) : QWidget(parent), m_ui(new Ui::FtbPage), m_dialog(dialog)
{
    m_ui->setupUi(this);

    m_filterModel = new Ftb::FilterModel(this);
    m_listModel = new Ftb::ListModel(this);
    m_filterModel->setSourceModel(m_listModel);
    m_ui->packView->setModel(m_filterModel);
    m_ui->packView->setSortingEnabled(true);
    m_ui->packView->header()->hide();
    m_ui->packView->setIndentation(0);

    m_ui->searchEdit->installEventFilter(this);

    m_ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

    for (int i = 0; i < m_filterModel->getAvailableSortings().size(); i++) {
        m_ui->sortByBox->addItem(m_filterModel->getAvailableSortings().keys().at(i));
    }
    m_ui->sortByBox->setCurrentText(m_filterModel->translateCurrentSorting());

    connect(m_ui->searchEdit, &QLineEdit::textChanged, this, &FtbPage::triggerSearch);
    connect(m_ui->sortByBox, &QComboBox::currentTextChanged, this, &FtbPage::onSortingSelectionChanged);
    connect(m_ui->packView->selectionModel(), &QItemSelectionModel::currentChanged, this, &FtbPage::onSelectionChanged);
    connect(m_ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &FtbPage::onVersionSelectionChanged);

    m_ui->packDescription->setMetaEntry("FTBPacks");
}

FtbPage::~FtbPage()
{
    delete m_ui;
}

bool FtbPage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_ui->searchEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return) {
            triggerSearch();
            keyEvent->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

bool FtbPage::shouldDisplay() const
{
    return true;
}

void FtbPage::retranslate()
{
    m_ui->retranslateUi(this);
}

void FtbPage::openedImpl()
{
    if (!m_initialised || m_listModel->wasAborted()) {
        m_listModel->request();
        m_initialised = true;
    }

    suggestCurrent();
}

void FtbPage::closedImpl()
{
    if (m_listModel->isMakingRequest())
        m_listModel->abortRequest();
}

void FtbPage::suggestCurrent()
{
    if (!isOpened) {
        return;
    }

    if (m_selectedVersion.isEmpty()) {
        m_dialog->setSuggestedPack();
        return;
    }

    m_dialog->setSuggestedPack(m_selected.name, m_selectedVersion, new FTB::PackInstallTask(m_selected, m_selectedVersion, this));
    for (auto art : m_selected.art) {
        if (art.type == "square") {
            auto editedLogoName = "ftb_" + m_selected.safeName;
            m_listModel->getLogo(m_selected.safeName, art.url,
                                 [this, editedLogoName](QString logo) { m_dialog->setSuggestedIconFromFile(logo, editedLogoName); });
        }
    }
}

void FtbPage::triggerSearch()
{
    m_filterModel->setSearchTerm(m_ui->searchEdit->text());
}

void FtbPage::onSortingSelectionChanged(QString data)
{
    auto toSet = m_filterModel->getAvailableSortings().value(data);
    m_filterModel->setSorting(toSet);
}

void FtbPage::onSelectionChanged(QModelIndex first, QModelIndex second)
{
    m_ui->versionSelectionBox->clear();

    if (!first.isValid()) {
        if (isOpened) {
            m_dialog->setSuggestedPack();
        }
        return;
    }

    m_selected = m_filterModel->data(first, Qt::UserRole).value<FTB::Modpack>();

    QString output = markdownToHTML(m_selected.description.toUtf8());
    m_ui->packDescription->setHtml(output);

    // reverse foreach, so that the newest versions are first
    for (auto i = m_selected.versions.size(); i--;) {
        m_ui->versionSelectionBox->addItem(m_selected.versions.at(i).name);
    }

    suggestCurrent();
}

void FtbPage::onVersionSelectionChanged(QString data)
{
    if (data.isNull() || data.isEmpty()) {
        m_selectedVersion = "";
        return;
    }

    m_selectedVersion = data;
    suggestCurrent();
}

QString FtbPage::getSerachTerm() const
{
    return m_ui->searchEdit->text();
}

void FtbPage::setSearchTerm(QString term)
{
    m_ui->searchEdit->setText(term);
}