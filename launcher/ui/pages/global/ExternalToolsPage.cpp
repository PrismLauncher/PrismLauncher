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
#include "config/GlobalConfig.h"
#include "ui_ExternalToolsPage.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTabBar>
#include <utility>

#include <FileSystem.h>
#include "Application.h"
#include "tools/BaseProfiler.h"

ExternalToolsPage::ExternalToolsPage(QWidget* parent) : QWidget(parent), ui(new Ui::ExternalToolsPage)
{
    ui->setupUi(this);

    ui->jsonEditorTextBox->setClearButtonEnabled(true);

    ui->jvisualvmLink->setOpenExternalLinks(true);
    ui->jprofilerLink->setOpenExternalLinks(true);
    loadSettings();
}

ExternalToolsPage::~ExternalToolsPage()
{
    delete ui;
}

void ExternalToolsPage::loadSettings()
{
    const auto& conf = *APPLICATION->config();
    ui->jprofilerPathEdit->setText(conf.jProfilerPath);
    ui->jvisualvmPathEdit->setText(conf.jVisualVmPath);

    // Editors
    ui->jsonEditorTextBox->setText(conf.jsonEditorPath);
}
void ExternalToolsPage::applySettings()
{
    auto& conf = APPLICATION->config().update();

    conf.jProfilerPath = ui->jprofilerPathEdit->text();
    conf.jVisualVmPath = ui->jvisualvmPathEdit->text();

    // Editors
    QString jsonEditor = ui->jsonEditorTextBox->text();
    if (!jsonEditor.isEmpty() && (!QFileInfo(jsonEditor).exists() || !QFileInfo(jsonEditor).isExecutable())) {
        QString found = QStandardPaths::findExecutable(jsonEditor);
        if (!found.isEmpty()) {
            jsonEditor = found;
        }
    }
    conf.jsonEditorPath = std::move(jsonEditor);
}

void ExternalToolsPage::on_jprofilerPathBtn_clicked()
{
    QString raw_dir = ui->jprofilerPathEdit->text();
    QString error;
    do {
        raw_dir = QFileDialog::getExistingDirectory(this, tr("JProfiler Folder"), raw_dir);
        if (raw_dir.isEmpty()) {
            break;
        }
        QString cooked_dir = FS::NormalizePath(raw_dir);
        if (!APPLICATION->profilers()["jprofiler"]->check(cooked_dir, &error)) {
            QMessageBox::critical(this, tr("Error"), tr("Error while checking JProfiler install:\n%1").arg(error));
            continue;
        } else {
            ui->jprofilerPathEdit->setText(cooked_dir);
            break;
        }
    } while (1);
}
void ExternalToolsPage::on_jprofilerCheckBtn_clicked()
{
    QString error;
    if (!APPLICATION->profilers()["jprofiler"]->check(ui->jprofilerPathEdit->text(), &error)) {
        QMessageBox::critical(this, tr("Error"), tr("Error while checking JProfiler install:\n%1").arg(error));
    } else {
        QMessageBox::information(this, tr("OK"), tr("JProfiler setup seems to be OK"));
    }
}

void ExternalToolsPage::on_jvisualvmPathBtn_clicked()
{
    QString raw_dir = ui->jvisualvmPathEdit->text();
    QString error;
    do {
        raw_dir = QFileDialog::getOpenFileName(this, tr("VisualVM Executable"), raw_dir);
        if (raw_dir.isEmpty()) {
            break;
        }
        QString cooked_dir = FS::NormalizePath(raw_dir);
        if (!APPLICATION->profilers()["jvisualvm"]->check(cooked_dir, &error)) {
            QMessageBox::critical(this, tr("Error"), tr("Error while checking VisualVM install:\n%1").arg(error));
            continue;
        } else {
            ui->jvisualvmPathEdit->setText(cooked_dir);
            break;
        }
    } while (1);
}
void ExternalToolsPage::on_jvisualvmCheckBtn_clicked()
{
    QString error;
    if (!APPLICATION->profilers()["jvisualvm"]->check(ui->jvisualvmPathEdit->text(), &error)) {
        QMessageBox::critical(this, tr("Error"), tr("Error while checking VisualVM install:\n%1").arg(error));
    } else {
        QMessageBox::information(this, tr("OK"), tr("VisualVM setup seems to be OK"));
    }
}

void ExternalToolsPage::on_jsonEditorBrowseBtn_clicked()
{
    QString raw_file = QFileDialog::getOpenFileName(this, tr("Text Editor"),
                                                    ui->jsonEditorTextBox->text().isEmpty()
#if defined(Q_OS_LINUX)
                                                        ? QString("/usr/bin")
#else
                                                        ? QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation).first()
#endif
                                                        : ui->jsonEditorTextBox->text());

    if (raw_file.isEmpty()) {
        return;
    }
    QString cooked_file = FS::NormalizePath(raw_file);

    // it has to exist and be an executable
    if (QFileInfo(cooked_file).exists() && QFileInfo(cooked_file).isExecutable()) {
        ui->jsonEditorTextBox->setText(cooked_file);
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
    ui->retranslateUi(this);
}
