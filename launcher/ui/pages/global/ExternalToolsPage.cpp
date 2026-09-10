// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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

#include "ExternalToolsPage.h"
#include "ui_ExternalToolsPage.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTabBar>

#include <FileSystem.h>
#include <QTreeWidgetItem>
#include "Application.h"
#include "Commandline.h"
#include "Json.h"
#include "settings/SettingsObject.h"
#include "tools/BaseProfiler.h"

ExternalToolsPage::ExternalToolsPage(QWidget* parent) : QWidget(parent), m_ui(new Ui::ExternalToolsPage)
{
    m_ui->setupUi(this);

    m_ui->jsonEditorTextBox->setClearButtonEnabled(true);

    m_ui->jvisualvmLink->setOpenExternalLinks(true);
    m_ui->jprofilerLink->setOpenExternalLinks(true);

    m_ui->worldToolTree->header()->setStretchLastSection(false);
    m_ui->worldToolTree->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_ui->worldToolTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_ui->worldToolTree->header()->setSectionResizeMode(2, QHeaderView::Interactive);
    m_ui->worldToolTree->header()->resizeSection(0, 150);
    m_ui->worldToolTree->header()->resizeSection(2, 36);
    loadSettings();
}

ExternalToolsPage::~ExternalToolsPage()
{
    delete m_ui;
}

void ExternalToolsPage::loadSettings()
{
    auto* s = APPLICATION->settings();
    m_ui->jprofilerPathEdit->setText(s->get("JProfilerPath").toString());
    m_ui->jvisualvmPathEdit->setText(s->get("JVisualVMPath").toString());

    // Editors
    m_ui->jsonEditorTextBox->setText(s->get("JsonEditor").toString());

    // World Tools
    m_ui->worldToolTree->clear();
    const QVariantMap tools = Json::toMap(APPLICATION->settings()->get("WorldTools").toString());
    for (auto it = tools.constBegin(); it != tools.constEnd(); ++it) {
        auto* item = new QTreeWidgetItem(m_ui->worldToolTree);
        item->setText(0, it.key());
        item->setText(1, it.value().toString());
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        m_ui->worldToolTree->addTopLevelItem(item);
        setupWorldToolBrowseBtn(item);
    }
}

void ExternalToolsPage::setupWorldToolBrowseBtn(QTreeWidgetItem* item)
{
    auto* btn = new QPushButton("...");
    btn->setFixedWidth(30);
    connect(btn, &QPushButton::clicked, this, [this, item]() {
#ifdef Q_OS_WIN
        const QString filter = tr("Executables (*.exe *.bat);;All Files (*)");
#else
        const QString filter = tr("All Files (*)");
#endif
        const QString filePath = QFileDialog::getOpenFileName(this, tr("Select Executable"), QString(), filter);
        if (!filePath.isEmpty()) {
            QFileInfo fileInfo(filePath);
            if (!fileInfo.isExecutable()) {
                QMessageBox::warning(this, tr("Invalid"), tr("The selected file is not executable"));
                return;
            }
            item->setText(1, Commandline::quoteForSplitCommand(filePath) + " ${WORLD_PATH}");
            if (item->text(0).trimmed().isEmpty()) {
                item->setText(0, fileInfo.baseName());
            }
        }
    });
    m_ui->worldToolTree->setItemWidget(item, 2, btn);
}
void ExternalToolsPage::applySettings()
{
    auto* s = APPLICATION->settings();

    s->set("JProfilerPath", m_ui->jprofilerPathEdit->text());
    s->set("JVisualVMPath", m_ui->jvisualvmPathEdit->text());

    // Editors
    QString jsonEditor = m_ui->jsonEditorTextBox->text();
    if (!jsonEditor.isEmpty() && (!QFileInfo(jsonEditor).exists() || !QFileInfo(jsonEditor).isExecutable())) {
        QString found = QStandardPaths::findExecutable(jsonEditor);
        if (!found.isEmpty()) {
            jsonEditor = found;
        }
    }
    s->set("JsonEditor", jsonEditor);

    // World Tools
    QVariantMap tools;
    auto* item = m_ui->worldToolTree->topLevelItem(0);
    for (int i = 1; item != nullptr; item = m_ui->worldToolTree->topLevelItem(i++)) {
        const QString name = item->text(0).trimmed();
        const QString command = item->text(1).trimmed();
        if (!name.isEmpty() && !command.isEmpty()) {
            tools.insert(name, command);
        }
    }
    APPLICATION->settings()->set("WorldTools", Json::fromMap(tools));
}

void ExternalToolsPage::on_jprofilerPathBtn_clicked()
{
    QString rawDir = m_ui->jprofilerPathEdit->text();
    QString error;
    do {
        rawDir = QFileDialog::getExistingDirectory(this, tr("JProfiler Folder"), rawDir);
        if (rawDir.isEmpty()) {
            break;
        }
        QString cookedDir = FS::NormalizePath(rawDir);
        if (!APPLICATION->profilers()["jprofiler"]->check(cookedDir, &error)) {
            QMessageBox::critical(this, tr("Error"), tr("Error while checking JProfiler install:\n%1").arg(error));
            continue;
        }
        m_ui->jprofilerPathEdit->setText(cookedDir);
        break;

    } while (true);
}
void ExternalToolsPage::on_jprofilerCheckBtn_clicked()
{
    QString error;
    if (!APPLICATION->profilers()["jprofiler"]->check(m_ui->jprofilerPathEdit->text(), &error)) {
        QMessageBox::critical(this, tr("Error"), tr("Error while checking JProfiler install:\n%1").arg(error));
    } else {
        QMessageBox::information(this, tr("OK"), tr("JProfiler setup seems to be OK"));
    }
}

void ExternalToolsPage::on_jvisualvmPathBtn_clicked()
{
    QString rawDir = m_ui->jvisualvmPathEdit->text();
    QString error;
    do {
        rawDir = QFileDialog::getOpenFileName(this, tr("VisualVM Executable"), rawDir);
        if (rawDir.isEmpty()) {
            break;
        }
        QString cookedDir = FS::NormalizePath(rawDir);
        if (!APPLICATION->profilers()["jvisualvm"]->check(cookedDir, &error)) {
            QMessageBox::critical(this, tr("Error"), tr("Error while checking VisualVM install:\n%1").arg(error));
            continue;
        }
        m_ui->jvisualvmPathEdit->setText(cookedDir);
        break;

    } while (true);
}
void ExternalToolsPage::on_jvisualvmCheckBtn_clicked()
{
    QString error;
    if (!APPLICATION->profilers()["jvisualvm"]->check(m_ui->jvisualvmPathEdit->text(), &error)) {
        QMessageBox::critical(this, tr("Error"), tr("Error while checking VisualVM install:\n%1").arg(error));
    } else {
        QMessageBox::information(this, tr("OK"), tr("VisualVM setup seems to be OK"));
    }
}

void ExternalToolsPage::on_worldToolAddBtn_clicked()
{
    auto* item = new QTreeWidgetItem(m_ui->worldToolTree);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    m_ui->worldToolTree->addTopLevelItem(item);
    setupWorldToolBrowseBtn(item);
    m_ui->worldToolTree->setCurrentItem(item);
    m_ui->worldToolTree->editItem(item, 0);
}

void ExternalToolsPage::on_worldToolRemoveBtn_clicked()
{
    for (QTreeWidgetItem* item : m_ui->worldToolTree->selectedItems()) {
        m_ui->worldToolTree->takeTopLevelItem(m_ui->worldToolTree->indexOfTopLevelItem(item));
    }
}

void ExternalToolsPage::on_jsonEditorBrowseBtn_clicked()
{
    QString rawFile = QFileDialog::getOpenFileName(this, tr("Text Editor"),
                                                   m_ui->jsonEditorTextBox->text().isEmpty()
#if defined(Q_OS_LINUX)
                                                       ? QString("/usr/bin")
#else
                                                       ? QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation).first()
#endif
                                                       : m_ui->jsonEditorTextBox->text());

    if (rawFile.isEmpty()) {
        return;
    }
    QString cookedFile = FS::NormalizePath(rawFile);

    // it has to exist and be an executable
    if (QFileInfo(cookedFile).exists() && QFileInfo(cookedFile).isExecutable()) {
        m_ui->jsonEditorTextBox->setText(cookedFile);
    } else {
        QMessageBox::warning(this, tr("Invalid"), tr("The file chosen does not seem to be an executable"));
    }
}

bool ExternalToolsPage::apply()
{
    applySettings();
    return true;
}

void ExternalToolsPage::retranslate()
{
    m_ui->retranslateUi(this);
}
