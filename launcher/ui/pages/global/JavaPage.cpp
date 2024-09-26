// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

#include "JavaPage.h"
#include "BuildConfig.h"
#include "JavaCommon.h"
#include "java/JavaInstall.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/java/InstallJavaDialog.h"
#include "ui_JavaPage.h"

#include <QCheckBox>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QStringListModel>
#include <QTabBar>

#include "ui/dialogs/VersionSelectDialog.h"

#include "java/JavaInstallList.h"
#include "java/JavaUtils.h"

#include <FileSystem.h>
#include <sys.h>
#include "Application.h"
#include "settings/SettingsObject.h"

JavaPage::JavaPage(QWidget* parent) : QWidget(parent), ui(new Ui::JavaPage)
{
    ui->setupUi(this);

    if (BuildConfig.JAVA_DOWNLOADER_ENABLED) {
        ui->managedJavaList->initialize(new JavaInstallList(this, true));
        ui->managedJavaList->setResizeOn(2);
        ui->managedJavaList->selectCurrent();
        ui->managedJavaList->setEmptyString(tr("No managed Java versions are installed"));
        ui->managedJavaList->setEmptyErrorString(tr("Couldn't load the managed Java list!"));
        connect(ui->autodetectJavaCheckBox, &QCheckBox::stateChanged, this, [this] {
            ui->autodownloadCheckBox->setEnabled(ui->autodetectJavaCheckBox->isChecked());
            if (!ui->autodetectJavaCheckBox->isChecked())
                ui->autodownloadCheckBox->setChecked(false);
        });
    } else {
        ui->autodownloadCheckBox->setHidden(true);
        ui->tabWidget->tabBar()->hide();
    }

    loadSettings();
    updateThresholds();
}

JavaPage::~JavaPage()
{
    delete ui;
}

bool JavaPage::apply()
{
    applySettings();
    return true;
}

void JavaPage::applySettings()
{
    auto s = APPLICATION->settings();

    // Memory
    int min = ui->minMemSpinBox->value();
    int max = ui->maxMemSpinBox->value();
    if (min < max) {
        s->set("MinMemAlloc", min);
        s->set("MaxMemAlloc", max);
    } else {
        s->set("MinMemAlloc", max);
        s->set("MaxMemAlloc", min);
    }
    s->set("PermGen", ui->permGenSpinBox->value());

    // Java Settings
    s->set("JavaPath", ui->javaPathTextBox->text());
    s->set("JvmArgs", ui->jvmArgsTextBox->toPlainText().replace("\n", " "));
    s->set("IgnoreJavaCompatibility", ui->skipCompatibilityCheckbox->isChecked());
    s->set("IgnoreJavaWizard", ui->skipJavaWizardCheckbox->isChecked());
    s->set("AutomaticJavaSwitch", ui->autodetectJavaCheckBox->isChecked());
    s->set("AutomaticJavaDownload", ui->autodownloadCheckBox->isChecked());
    JavaCommon::checkJVMArgs(s->get("JvmArgs").toString(), this->parentWidget());
}
void JavaPage::loadSettings()
{
    auto s = APPLICATION->settings();
    // Memory
    int min = s->get("MinMemAlloc").toInt();
    int max = s->get("MaxMemAlloc").toInt();
    if (min < max) {
        ui->minMemSpinBox->setValue(min);
        ui->maxMemSpinBox->setValue(max);
    } else {
        ui->minMemSpinBox->setValue(max);
        ui->maxMemSpinBox->setValue(min);
    }
    ui->permGenSpinBox->setValue(s->get("PermGen").toInt());

    // Java Settings
    ui->javaPathTextBox->setText(s->get("JavaPath").toString());
    ui->jvmArgsTextBox->setPlainText(s->get("JvmArgs").toString());
    ui->skipCompatibilityCheckbox->setChecked(s->get("IgnoreJavaCompatibility").toBool());
    ui->skipJavaWizardCheckbox->setChecked(s->get("IgnoreJavaWizard").toBool());
    ui->autodetectJavaCheckBox->setChecked(s->get("AutomaticJavaSwitch").toBool());
    ui->autodownloadCheckBox->setChecked(s->get("AutomaticJavaSwitch").toBool() && s->get("AutomaticJavaDownload").toBool());
}

void JavaPage::on_javaDetectBtn_clicked()
{
    if (JavaUtils::getJavaCheckPath().isEmpty()) {
        JavaCommon::javaCheckNotFound(this);
        return;
    }

    JavaInstallPtr java;

    VersionSelectDialog vselect(APPLICATION->javalist().get(), tr("Select a Java version"), this, true);
    vselect.setResizeOn(2);
    vselect.exec();

    if (vselect.result() == QDialog::Accepted && vselect.selectedVersion()) {
        java = std::dynamic_pointer_cast<JavaInstall>(vselect.selectedVersion());
        ui->javaPathTextBox->setText(java->path);
        if (!java->is_64bit && APPLICATION->settings()->get("MaxMemAlloc").toInt() > 2048) {
            CustomMessageBox::selectable(this, tr("Confirm Selection"),
                                         tr("You selected a 32-bit version of Java.\n"
                                            "This installation does not support more than 2048MiB of RAM.\n"
                                            "Please make sure that the maximum memory value is lower."),
                                         QMessageBox::Warning, QMessageBox::Ok, QMessageBox::Ok)
                ->exec();
        }
    }
}

void JavaPage::on_javaBrowseBtn_clicked()
{
    QString raw_path = QFileDialog::getOpenFileName(this, tr("Find Java executable"));

    // do not allow current dir - it's dirty. Do not allow dirs that don't exist
    if (raw_path.isEmpty()) {
        return;
    }

    QString cooked_path = FS::NormalizePath(raw_path);
    QFileInfo javaInfo(cooked_path);
    ;
    if (!javaInfo.exists() || !javaInfo.isExecutable()) {
        return;
    }
    ui->javaPathTextBox->setText(cooked_path);
}

void JavaPage::on_javaTestBtn_clicked()
{
    if (checker) {
        return;
    }
    checker.reset(new JavaCommon::TestCheck(this, ui->javaPathTextBox->text(), ui->jvmArgsTextBox->toPlainText().replace("\n", " "),
                                            ui->minMemSpinBox->value(), ui->maxMemSpinBox->value(), ui->permGenSpinBox->value()));
    connect(checker.get(), SIGNAL(finished()), SLOT(checkerFinished()));
    checker->run();
}

void JavaPage::on_downloadJavaButton_clicked()
{
    auto jdialog = new Java::InstallDialog({}, nullptr, this);
    jdialog->exec();
    ui->managedJavaList->loadList();
}

void JavaPage::on_maxMemSpinBox_valueChanged([[maybe_unused]] int i)
{
    updateThresholds();
}

void JavaPage::checkerFinished()
{
    checker.reset();
}

void JavaPage::retranslate()
{
    ui->retranslateUi(this);
}

void JavaPage::updateThresholds()
{
    auto sysMiB = Sys::getSystemRam() / Sys::mebibyte;
    unsigned int maxMem = ui->maxMemSpinBox->value();
    unsigned int minMem = ui->minMemSpinBox->value();

    QString iconName;

    if (maxMem >= sysMiB) {
        iconName = "status-bad";
        ui->labelMaxMemIcon->setToolTip(tr("Your maximum memory allocation exceeds your system memory capacity."));
    } else if (maxMem > (sysMiB * 0.9)) {
        iconName = "status-yellow";
        ui->labelMaxMemIcon->setToolTip(tr("Your maximum memory allocation approaches your system memory capacity."));
    } else if (maxMem < minMem) {
        iconName = "status-yellow";
        ui->labelMaxMemIcon->setToolTip(tr("Your maximum memory allocation is smaller than the minimum value"));
    } else {
        iconName = "status-good";
        ui->labelMaxMemIcon->setToolTip("");
    }

    {
        auto height = ui->labelMaxMemIcon->fontInfo().pixelSize();
        QIcon icon = APPLICATION->getThemedIcon(iconName);
        QPixmap pix = icon.pixmap(height, height);
        ui->labelMaxMemIcon->setPixmap(pix);
    }
}

void JavaPage::on_removeJavaButton_clicked()
{
    auto version = ui->managedJavaList->selectedVersion();
    auto dcast = std::dynamic_pointer_cast<JavaInstall>(version);
    if (!dcast) {
        return;
    }
    QDir dir(APPLICATION->javaPath());

    auto entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (auto& entry : entries) {
        if (dcast->path.startsWith(entry.canonicalFilePath())) {
            auto response = CustomMessageBox::selectable(this, tr("Confirm Deletion"),
                                                         tr("You are about to remove  the Java installation named \"%1\".\n"
                                                            "Are you sure?")
                                                             .arg(entry.fileName()),
                                                         QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                                ->exec();

            if (response == QMessageBox::Yes) {
                FS::deletePath(entry.canonicalFilePath());
                ui->managedJavaList->loadList();
            }
            break;
        }
    }
}
void JavaPage::on_refreshJavaButton_clicked()
{
    ui->managedJavaList->loadList();
}
