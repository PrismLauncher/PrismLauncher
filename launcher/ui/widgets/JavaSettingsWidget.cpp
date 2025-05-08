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
#include <QTabBar>
#include "Application.h"
#include "BuildConfig.h"
#include "JavaCommon.h"

#include "Json.h"
#include "ui/widgets/JavaProfileSettingsWidget.h"
#include "ui_JavaSettingsWidget.h"

JavaSettingsWidget::JavaSettingsWidget(QWidget* parent) : QWidget(parent), m_ui(new Ui::JavaSettingsWidget)
{
    m_ui->setupUi(this);

    auto defaultProfile = new JavaProfileSettingsWidget("", m_ui->javaProfiles);
    m_ui->javaProfiles->addTab(defaultProfile, "Default");
    m_profiles << defaultProfile;
    if (BuildConfig.JAVA_DOWNLOADER_ENABLED) {
        connect(m_ui->autodetectJavaCheckBox, &QCheckBox::stateChanged, this, [this](bool state) {
            m_ui->autodownloadJavaCheckBox->setEnabled(state);
            if (!state)
                m_ui->autodownloadJavaCheckBox->setChecked(false);
        });
    } else {
        m_ui->autodownloadJavaCheckBox->hide();
    }
    for (auto major : Json::toStringList(APPLICATION->settings()->get("SupportedJavaMajors").toString())) {
        auto profileSetting = new JavaProfileSettingsWidget(major, m_ui->javaProfiles);
        m_ui->javaProfiles->addTab(profileSetting, tr("Java %1").arg(major));
        m_profiles << profileSetting;
    }

    loadSettings();
}

JavaSettingsWidget::~JavaSettingsWidget()
{
    delete m_ui;
}

void JavaSettingsWidget::loadSettings()
{
    auto settings = APPLICATION->settings();

    // Java Settings
    m_ui->skipWizardCheckBox->setChecked(settings->get("IgnoreJavaWizard").toBool());
    m_ui->autodetectJavaCheckBox->setChecked(settings->get("AutomaticJavaSwitch").toBool());
    m_ui->autodetectJavaCheckBox->stateChanged(m_ui->autodetectJavaCheckBox->isChecked());
    m_ui->autodownloadJavaCheckBox->setChecked(settings->get("AutomaticJavaDownload").toBool());
}

void JavaSettingsWidget::saveSettings()
{
    auto settings = APPLICATION->settings();

    SettingsObject::Lock lock(settings);

    // Java Install Settings
    settings->set("IgnoreJavaWizard", m_ui->skipWizardCheckBox->isChecked());
    settings->set("AutomaticJavaSwitch", m_ui->autodetectJavaCheckBox->isChecked());
    settings->set("AutomaticJavaDownload", m_ui->autodownloadJavaCheckBox->isChecked());
    for (auto p : m_profiles) {
        p->saveSettings();
    }
}
