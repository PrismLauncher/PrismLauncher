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

#include "JavaSettingsWidget.h"

#include <QFileDialog>
#include <QFileInfo>
#include "Application.h"
#include "BuildConfig.h"
#include "FileSystem.h"
#include "HardwareInfo.h"
#include "JavaCommon.h"
#include "SysInfo.h"
#include "config/GlobalConfig.h"
#include "config/InstanceConfig.h"
#include "java/JavaInstallList.h"
#include "java/JavaUtils.h"
#include "minecraft/MinecraftInstance.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/VersionSelectDialog.h"
#include "ui/java/InstallJavaDialog.h"

#include "ui_JavaSettingsWidget.h"

JavaSettingsWidget::JavaSettingsWidget(MinecraftInstance* instance, QWidget* parent)
    : QWidget(parent), m_instance(instance), m_ui(new Ui::JavaSettingsWidget)
{
    m_ui->setupUi(this);

    if (m_instance == nullptr) {
        m_ui->javaDownloadBtn->hide();
        if (BuildConfig.JAVA_DOWNLOADER_ENABLED) {
            connect(m_ui->autodetectJavaCheckBox, &QCheckBox::stateChanged, this, [this](bool state) {
                m_ui->autodownloadJavaCheckBox->setEnabled(state);
                if (!state)
                    m_ui->autodownloadJavaCheckBox->setChecked(false);
            });
        } else {
            m_ui->autodownloadJavaCheckBox->hide();
        }
    } else {
        m_ui->javaDownloadBtn->setVisible(BuildConfig.JAVA_DOWNLOADER_ENABLED);
        m_ui->skipWizardCheckBox->hide();
        m_ui->autodetectJavaCheckBox->hide();
        m_ui->autodownloadJavaCheckBox->hide();

        m_ui->javaInstallationGroupBox->setCheckable(true);
        m_ui->memoryGroupBox->setCheckable(true);
        m_ui->javaArgumentsGroupBox->setCheckable(true);

        connect(&m_instance->config(), &InstanceConfigHolder::updated, this, [this] {
            const auto& conf = *m_instance->config();

            m_ui->javaInstallationGroupBox->setChecked(conf.javaInstallation.has_value());

            const auto installation = conf.javaInstallationOrGlobal(*APPLICATION->config());
            m_ui->javaPathTextBox->setText(installation.path);
        });

        connect(m_ui->javaDownloadBtn, &QPushButton::clicked, this, [this] {
            auto javaDialog = new Java::InstallDialog({}, m_instance, this);
            javaDialog->exec();
        });
        connect(m_ui->javaPathTextBox, &QLineEdit::textChanged, this, [this](QString newValue) {
            const auto& installation = m_instance->config()->javaInstallation;
            if (installation.has_value() && installation->path != newValue) {
                m_instance->config().update().automaticJava = false;
            }
        });
    }

    connect(m_ui->javaTestBtn, &QPushButton::clicked, this, &JavaSettingsWidget::onJavaTest);
    connect(m_ui->javaDetectBtn, &QPushButton::clicked, this, &JavaSettingsWidget::onJavaAutodetect);
    connect(m_ui->javaBrowseBtn, &QPushButton::clicked, this, &JavaSettingsWidget::onJavaBrowse);

    connect(m_ui->maxMemSpinBox, &QSpinBox::valueChanged, this, &JavaSettingsWidget::updateThresholds);
    connect(m_ui->minMemSpinBox, &QSpinBox::valueChanged, this, &JavaSettingsWidget::updateThresholds);

    loadSettings();
    updateThresholds();
}

JavaSettingsWidget::~JavaSettingsWidget()
{
    delete m_ui;
}

void JavaSettingsWidget::loadSettings()
{
    if (m_instance != nullptr) {
        const auto& conf = *m_instance->config();

        loadSettingsFrom(conf, *APPLICATION->config());

        m_ui->javaInstallationGroupBox->setChecked(conf.javaInstallation.has_value());
        m_ui->javaArgumentsGroupBox->setChecked(conf.jvmArgs.has_value());
        m_ui->memoryGroupBox->setChecked(conf.memory.has_value());
    } else {
        const auto& conf = *APPLICATION->config();

        loadSettingsFrom(conf, conf);

        // global-only
        m_ui->skipWizardCheckBox->setChecked(conf.ignoreJavaWizard);
        m_ui->autodetectJavaCheckBox->setChecked(conf.automaticJavaSwitch);
        m_ui->autodetectJavaCheckBox->stateChanged(m_ui->autodetectJavaCheckBox->isChecked());
        m_ui->autodownloadJavaCheckBox->setChecked(conf.automaticJavaDownload);
    }
}

template <typename T>
void JavaSettingsWidget::loadSettingsFrom(const T& conf, const GlobalConfig& global)
{
    // Java Settings
    const auto installation = conf.javaInstallationOrGlobal(global);
    m_ui->javaPathTextBox->setText(installation.path);
    m_ui->skipCompatibilityCheckBox->setChecked(installation.ignoreCompatibility);

    m_ui->jvmArgsTextBox->setPlainText(conf.jvmArgsOrGlobal(global));

    // Memory
    const auto memory = conf.memoryOrGlobal(global);
    const int min = std::min(memory.minAlloc, memory.maxAlloc);
    const int max = std::max(memory.minAlloc, memory.maxAlloc);
    m_ui->minMemSpinBox->setValue(min);
    m_ui->maxMemSpinBox->setValue(max);

    m_ui->permGenSpinBox->setValue(memory.permGen);
    m_ui->lowMemWarningCheckBox->setChecked(memory.lowMemWarning);
}

void JavaSettingsWidget::saveSettings()
{
    if (m_instance == nullptr) {
        auto& conf = APPLICATION->config().update();

        conf.javaInstallation.path = m_ui->javaPathTextBox->text();
        conf.javaInstallation.ignoreCompatibility = m_ui->skipCompatibilityCheckBox->isChecked();

        saveSettingsTo(conf);

        conf.ignoreJavaWizard = m_ui->skipWizardCheckBox->isChecked();
        conf.automaticJavaSwitch = m_ui->autodetectJavaCheckBox->isChecked();
        conf.automaticJavaDownload = m_ui->autodownloadJavaCheckBox->isChecked();
    } else {
        auto& conf = m_instance->config().update();

        if (m_ui->javaInstallationGroupBox->isChecked()) {
            if (!conf.javaInstallation.has_value()) {
                conf.javaInstallation = GlobalConfig::JavaInstallationOverrides{};
            }
            conf.javaInstallation->path = m_ui->javaPathTextBox->text();
            conf.javaInstallation->ignoreCompatibility = m_ui->skipCompatibilityCheckBox->isChecked();
        } else {
            conf.javaInstallation = std::nullopt;
        }

        conf.memory = std::nullopt;
        conf.jvmArgs = std::nullopt;

        saveSettingsTo(conf);
    }
}

template <typename T>
void JavaSettingsWidget::saveSettingsTo(T& conf) const
{
    if (!m_ui->memoryGroupBox->isCheckable() || m_ui->memoryGroupBox->isChecked()) {
        const int min = std::min(m_ui->minMemSpinBox->value(), m_ui->maxMemSpinBox->value());
        const int max = std::max(m_ui->minMemSpinBox->value(), m_ui->maxMemSpinBox->value());
        conf.memory = GlobalConfig::MemoryOverrides{
            .minAlloc = min,
            .maxAlloc = max,
            .permGen = m_ui->permGenSpinBox->value(),
            .lowMemWarning = m_ui->lowMemWarningCheckBox->isChecked(),
        };
    }

    if (!m_ui->javaArgumentsGroupBox->isCheckable() || m_ui->javaArgumentsGroupBox->isChecked()) {
        conf.jvmArgs = m_ui->jvmArgsTextBox->toPlainText().replace("\n", " ");
    }
}

void JavaSettingsWidget::onJavaBrowse()
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

void JavaSettingsWidget::onJavaTest()
{
    if (m_checker != nullptr)
        return;

    QString jvmArgs;

    if (m_instance == nullptr || m_ui->javaArgumentsGroupBox->isChecked())
        jvmArgs = m_ui->jvmArgsTextBox->toPlainText().replace("\n", " ");
    else
        jvmArgs = APPLICATION->config()->jvmArgs;

    m_checker.reset(new JavaCommon::TestCheck(this, m_ui->javaPathTextBox->text(), jvmArgs, m_ui->minMemSpinBox->value(),
                                              m_ui->maxMemSpinBox->value(), m_ui->permGenSpinBox->value()));
    connect(m_checker.get(), &JavaCommon::TestCheck::finished, this, [this] { m_checker.reset(); });
    m_checker->run();
}

void JavaSettingsWidget::onJavaAutodetect()
{
    if (JavaUtils::getJavaCheckPath().isEmpty()) {
        JavaCommon::javaCheckNotFound(this);
        return;
    }

    VersionSelectDialog versionDialog(APPLICATION->javalist(), tr("Select a Java version"), this, true);
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
void JavaSettingsWidget::updateThresholds()
{
    auto sysMiB = HardwareInfo::totalRamMiB();
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
