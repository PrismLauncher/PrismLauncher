// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include "JavaProfileSettingsWidget.h"

#include <QFileDialog>
#include <QFileInfo>
#include "Application.h"
#include "BuildConfig.h"
#include "FileSystem.h"
#include "JavaCommon.h"
#include "java/JavaInstallList.h"
#include "java/JavaUtils.h"
#include "settings/Setting.h"
#include "sys.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/VersionSelectDialog.h"
#include "ui/java/InstallJavaDialog.h"

#include "ui_JavaProfileSettingsWidget.h"

JavaProfileSettingsWidget::JavaProfileSettingsWidget(QString major, QWidget* parent) : JavaProfileSettingsWidget(nullptr, major, parent) {}
JavaProfileSettingsWidget::JavaProfileSettingsWidget(InstancePtr instance, QWidget* parent)
    : JavaProfileSettingsWidget(instance, {}, parent)
{}

JavaProfileSettingsWidget::JavaProfileSettingsWidget(InstancePtr instance, QString major, QWidget* parent)

    : QWidget(parent), m_major(major), m_instance(instance), m_ui(new Ui::JavaProfileSettingsWidget)
{
    m_ui->setupUi(this);

    auto overide = m_major.isEmpty() && m_instance == nullptr;
    m_ui->javaInstallationGroupBox->setCheckable(!overide);
    m_ui->memoryGroupBox->setCheckable(!overide);
    m_ui->javaArgumentsGroupBox->setCheckable(!overide);
    if (m_instance == nullptr) {
        m_ui->javaDownloadBtn->hide();
        if (!m_major.isEmpty()) {
            m_ui->javaInstallationGroupBox->setTitle(tr("Java %1 Insta&llation").arg(m_major));
        }
    } else {
        m_ui->javaDownloadBtn->setVisible(BuildConfig.JAVA_DOWNLOADER_ENABLED);

        SettingsObjectPtr settings = m_instance->settings();

        connect(settings->getSetting("OverrideJavaLocation").get(), &Setting::SettingChanged, m_ui->javaInstallationGroupBox,
                [this, settings] { m_ui->javaInstallationGroupBox->setChecked(settings->get("OverrideJavaLocation").toBool()); });
        connect(settings->getSetting("JavaPath").get(), &Setting::SettingChanged, m_ui->javaInstallationGroupBox,
                [this, settings] { m_ui->javaPathTextBox->setText(settings->get("JavaPath").toString()); });
        connect(m_ui->javaDownloadBtn, &QPushButton::clicked, this, [this] {
            auto javaDialog = new Java::InstallDialog({}, m_instance.get(), this);
            javaDialog->exec();
        });
        connect(m_ui->javaPathTextBox, &QLineEdit::textChanged, [this](QString newValue) {
            if (m_instance->settings()->get("JavaPath").toString() != newValue) {
                m_instance->settings()->set("AutomaticJava", false);
            }
        });
    }

    connect(m_ui->javaTestBtn, &QPushButton::clicked, this, &JavaProfileSettingsWidget::onJavaTest);
    connect(m_ui->javaDetectBtn, &QPushButton::clicked, this, &JavaProfileSettingsWidget::onJavaAutodetect);
    connect(m_ui->javaBrowseBtn, &QPushButton::clicked, this, &JavaProfileSettingsWidget::onJavaBrowse);

    connect(m_ui->maxMemSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &JavaProfileSettingsWidget::updateThresholds);
    connect(m_ui->minMemSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &JavaProfileSettingsWidget::updateThresholds);

    loadSettings();
    updateThresholds();
}

JavaProfileSettingsWidget::~JavaProfileSettingsWidget()
{
    delete m_ui;
}

void JavaProfileSettingsWidget::loadSettings()
{
    SettingsObjectPtr settings;

    if (m_instance != nullptr)
        settings = m_instance->settings();
    else
        settings = APPLICATION->settings();
    auto overide = m_major.isEmpty() && m_instance == nullptr;

    // Java Settings
    m_ui->javaInstallationGroupBox->setChecked(overide || settings->get(QString("OverrideJava%1Location").arg(m_major)).toBool());
    m_ui->javaPathTextBox->setText(settings->get(QString("Java%1Path").arg(m_major)).toString());

    m_ui->skipCompatibilityCheckBox->setChecked(settings->get(QString("IgnoreJava%1Compatibility").arg(m_major)).toBool());

    m_ui->javaArgumentsGroupBox->setChecked(overide || settings->get(QString("OverrideJava%1Args").arg(m_major)).toBool());
    m_ui->jvmArgsTextBox->setPlainText(settings->get(QString("Jvm%1Args").arg(m_major)).toString());

    // Memory
    m_ui->memoryGroupBox->setChecked(overide || settings->get(QString("OverrideMemory%1").arg(m_major)).toBool());
    int min = settings->get(QString("MinMemAlloc%1").arg(m_major)).toInt();
    int max = settings->get(QString("MaxMemAlloc%1").arg(m_major)).toInt();
    if (min < max) {
        m_ui->minMemSpinBox->setValue(min);
        m_ui->maxMemSpinBox->setValue(max);
    } else {
        m_ui->minMemSpinBox->setValue(max);
        m_ui->maxMemSpinBox->setValue(min);
    }
    m_ui->permGenSpinBox->setValue(settings->get(QString("PermGen%1").arg(m_major)).toInt());

    // Java arguments
    m_ui->javaArgumentsGroupBox->setChecked(overide || settings->get(QString("OverrideJava%1Args").arg(m_major)).toBool());
    m_ui->jvmArgsTextBox->setPlainText(settings->get(QString("Jvm%1Args").arg(m_major)).toString());
}

void JavaProfileSettingsWidget::saveSettings()
{
    SettingsObjectPtr settings;

    if (m_instance != nullptr)
        settings = m_instance->settings();
    else
        settings = APPLICATION->settings();

    SettingsObject::Lock lock(settings);

    // Java Install Settings
    auto overide = m_major.isEmpty() && m_instance == nullptr;
    bool javaInstall = overide || m_ui->javaInstallationGroupBox->isChecked();

    if (!overide)
        settings->set(QString("OverrideJava%1Location").arg(m_major), javaInstall);

    if (javaInstall) {
        settings->set(QString("Java%1Path").arg(m_major), m_ui->javaPathTextBox->text());
        settings->set(QString("IgnoreJava%1Compatibility").arg(m_major), m_ui->skipCompatibilityCheckBox->isChecked());
    } else {
        settings->reset(QString("Java%1Path").arg(m_major));
        settings->reset(QString("IgnoreJava%1Compatibility").arg(m_major));
    }

    // Memory
    bool memory = overide || m_ui->memoryGroupBox->isChecked();

    if (!overide)
        settings->set(QString("OverrideMemory%1").arg(m_major), memory);

    if (memory) {
        int min = m_ui->minMemSpinBox->value();
        int max = m_ui->maxMemSpinBox->value();
        if (min < max) {
            settings->set(QString("MinMemAlloc%1").arg(m_major), min);
            settings->set(QString("MaxMemAlloc%1").arg(m_major), max);
        } else {
            settings->set(QString("MinMemAlloc%1").arg(m_major), max);
            settings->set(QString("MaxMemAlloc%1").arg(m_major), min);
        }
        settings->set(QString("PermGen%1").arg(m_major), m_ui->permGenSpinBox->value());
    } else {
        settings->reset(QString("MinMemAlloc%1").arg(m_major));
        settings->reset(QString("MaxMemAlloc%1").arg(m_major));
        settings->reset(QString("PermGen%1").arg(m_major));
    }

    // Java arguments
    bool javaArgs = overide || m_ui->javaArgumentsGroupBox->isChecked();

    if (!overide)
        settings->set(QString("OverrideJava%1Args").arg(m_major), javaArgs);

    if (javaArgs) {
        settings->set(QString("Jvm%1Args").arg(m_major), m_ui->jvmArgsTextBox->toPlainText().replace("\n", " "));
    } else {
        settings->reset(QString("Jvm%1Args").arg(m_major));
    }
}

void JavaProfileSettingsWidget::onJavaBrowse()
{
    QString rawPath = QFileDialog::getOpenFileName(this, tr("Find Java executable"));

    // do not allow current dir - it's dirty. Do not allow dirs that don't exist
    if (rawPath.isEmpty()) {
        return;
    }

    QString cookedPath = FS::NormalizePath(rawPath);
    QFileInfo javaInfo(cookedPath);
    if (!javaInfo.exists() || !javaInfo.isExecutable()) {
        return;
    }
    m_ui->javaPathTextBox->setText(cookedPath);
}

void JavaProfileSettingsWidget::onJavaTest()
{
    if (m_checker != nullptr)
        return;

    QString jvmArgs;

    if (m_major.isEmpty() || m_ui->javaArgumentsGroupBox->isChecked())
        jvmArgs = m_ui->jvmArgsTextBox->toPlainText().replace("\n", " ");
    else
        jvmArgs = APPLICATION->settings()->get("JvmArgs").toString();

    m_checker.reset(new JavaCommon::TestCheck(this, m_ui->javaPathTextBox->text(), jvmArgs, m_ui->minMemSpinBox->value(),
                                              m_ui->maxMemSpinBox->value(), m_ui->permGenSpinBox->value()));
    connect(m_checker.get(), &JavaCommon::TestCheck::finished, this, [this] { m_checker.reset(); });
    m_checker->run();
}

void JavaProfileSettingsWidget::onJavaAutodetect()
{
    if (JavaUtils::getJavaCheckPath().isEmpty()) {
        JavaCommon::javaCheckNotFound(this);
        return;
    }

    VersionSelectDialog versionDialog(APPLICATION->javalist().get(), tr("Select a Java version"), this, true);
    versionDialog.setResizeOn(2);
    versionDialog.exec();

    if (versionDialog.result() == QDialog::Accepted && versionDialog.selectedVersion()) {
        JavaInstallPtr java = std::dynamic_pointer_cast<JavaInstall>(versionDialog.selectedVersion());
        m_ui->javaPathTextBox->setText(java->path);

        if (!java->is_64bit && m_ui->maxMemSpinBox->value() > 2048) {
            CustomMessageBox::selectable(this, tr("Confirm Selection"),
                                         tr("You selected a 32-bit version of Java.\n"
                                            "This installation does not support more than 2048MiB of RAM.\n"
                                            "Please make sure that the maximum memory value is lower."),
                                         QMessageBox::Warning, QMessageBox::Ok, QMessageBox::Ok)
                ->exec();
        }
    }
}
void JavaProfileSettingsWidget::updateThresholds()
{
    auto sysMiB = Sys::getSystemRam() / Sys::mebibyte;
    unsigned int maxMem = m_ui->maxMemSpinBox->value();
    unsigned int minMem = m_ui->minMemSpinBox->value();

    const QString warningColour(QStringLiteral("<span style='color:#f5c211'>%1</span>"));

    if (maxMem >= sysMiB) {
        m_ui->labelMaxMemNotice->setText(
            QString("<span style='color:red'>%1</span>").arg(tr("Your maximum memory allocation exceeds your system memory capacity.")));
        m_ui->labelMaxMemNotice->show();
    } else if (maxMem > (sysMiB * 0.9)) {
        m_ui->labelMaxMemNotice->setText(warningColour.arg(tr("Your maximum memory allocation is close to your system memory capacity.")));
        m_ui->labelMaxMemNotice->show();
    } else if (maxMem < minMem) {
        m_ui->labelMaxMemNotice->setText(warningColour.arg(tr("Your maximum memory allocation is below the minimum memory allocation.")));
        m_ui->labelMaxMemNotice->show();
    } else {
        m_ui->labelMaxMemNotice->hide();
    }
}
