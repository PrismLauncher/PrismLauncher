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

#include "CustomPage.h"
#include "ui_CustomPage.h"

#include <QTabBar>
#include <utility>

#include "Application.h"
#include "Filter.h"
#include "meta/Index.h"
#include "minecraft/VanillaInstanceCreationTask.h"
#include "ui/dialogs/InstallLoaderDialog.h"
#include "ui/dialogs/NewInstanceDialog.h"

using namespace Qt::Literals;

CustomPage::CustomPage(NewInstanceDialog* dialog, QWidget* parent) : QWidget(parent), m_dialog(dialog), m_ui(new Ui::CustomPage)
{
    m_ui->setupUi(this);
    connect(m_ui->versionList, &VersionSelectWidget::selectedVersionChanged, this, &CustomPage::setSelectedVersion);
    filterChanged();
    connect(m_ui->alphaFilter, &QCheckBox::checkStateChanged, this, &CustomPage::filterChanged);
    connect(m_ui->betaFilter, &QCheckBox::checkStateChanged, this, &CustomPage::filterChanged);
    connect(m_ui->snapshotFilter, &QCheckBox::checkStateChanged, this, &CustomPage::filterChanged);
    connect(m_ui->releaseFilter, &QCheckBox::checkStateChanged, this, &CustomPage::filterChanged);
    connect(m_ui->experimentsFilter, &QCheckBox::checkStateChanged, this, &CustomPage::filterChanged);
    connect(m_ui->refreshBtn, &QPushButton::clicked, this, &CustomPage::refresh);

    loaderChanged();
    connect(m_ui->installLoaderButton, &QPushButton::clicked, this, &CustomPage::chooseLoader);
    connect(m_ui->replaceLoaderButton, &QPushButton::clicked, this, &CustomPage::chooseLoader);
    connect(m_ui->removeLoaderButton, &QPushButton::clicked, this, &CustomPage::clearLoader);
}

void CustomPage::openedImpl()
{
    if (!m_initialized) {
        auto vlist = APPLICATION->metadataIndex()->get("net.minecraft");
        m_ui->versionList->initialize(vlist.get());
        m_initialized = true;
    } else {
        suggestCurrent();
    }
}

void CustomPage::refresh()
{
    m_ui->versionList->loadList(true);
}

void CustomPage::filterChanged()
{
    QStringList out;
    if (m_ui->alphaFilter->isChecked()) {
        out << "(alpha)";
    }
    if (m_ui->betaFilter->isChecked()) {
        out << "(beta)";
    }
    if (m_ui->snapshotFilter->isChecked()) {
        out << "(snapshot)";
    }
    if (m_ui->releaseFilter->isChecked()) {
        out << "(release)";
    }
    if (m_ui->experimentsFilter->isChecked()) {
        out << "(experiment)";
    }
    auto regexp = out.join('|');
    m_ui->versionList->setFilter(BaseVersionList::TypeRole, Filters::regexp(QRegularExpression(regexp)));
}

void CustomPage::loaderChanged()
{
    if (m_selectedLoader != nullptr) {
        if (m_selectedLoaderVersion != nullptr) {
            m_ui->loaderLabel->setText(tr("Using %1 %2").arg(m_selectedLoader->name(), m_selectedLoaderVersion->version()));
        } else {
            m_ui->loaderLabel->setText(tr("Using %1").arg(m_selectedLoader->name()));
        }
    } else {
        m_ui->loaderLabel->setText(tr("Using Vanilla Minecraft"));
    }

    m_ui->installLoaderButton->setVisible(m_selectedLoader == nullptr);
    m_ui->replaceLoaderButton->setVisible(m_selectedLoader != nullptr);
    m_ui->removeLoaderButton->setVisible(m_selectedLoader != nullptr);
}

void CustomPage::chooseLoader()
{
    const auto version = InstallLoaderDialog::choose(m_selectedVersion->descriptor(), this);
    if (!version.has_value()) {
        return;
    }

    m_selectedLoader = version->list;
    m_selectedLoaderVersion = version->version;
    loaderChanged();
    suggestCurrent();
}

void CustomPage::clearLoader()
{
    m_selectedLoader = nullptr;
    m_selectedLoaderVersion = nullptr;
    loaderChanged();
    suggestCurrent();
}

void CustomPage::syncLoader(const QString& gameVersion)
{
    if (m_selectedLoader == nullptr) {
        return;
    }

    auto isLoaderCompatible = [](const Meta::Version& version, const QString& gameVersion) {
        const auto& requiredSet = version.requiredSet();
        const auto& gameVersionReq = std::ranges::find_if(requiredSet, [](const Meta::Require& req) { return req.uid == "net.minecraft"; });
        if (gameVersionReq == requiredSet.end()) {
            return true;
        }
        if (gameVersionReq->equalsVersion.isEmpty()) {
            return true;
        }

        return gameVersionReq->equalsVersion == gameVersion;
    };

    if (m_selectedLoaderVersion != nullptr && isLoaderCompatible(*m_selectedLoaderVersion, gameVersion)) {
        return;
    }

    m_selectedLoaderVersion = m_selectedLoader->getRecommendedForParent("net.minecraft", gameVersion);
    if (m_selectedLoaderVersion == nullptr) {
        m_selectedLoaderVersion = m_selectedLoader->getLatestForParent("net.minecraft", gameVersion);
    }

    loaderChanged();
}

CustomPage::~CustomPage()
{
    delete m_ui;
}

bool CustomPage::shouldDisplay() const
{
    return true;
}

void CustomPage::retranslate()
{
    m_ui->retranslateUi(this);
}

void CustomPage::suggestCurrent()
{
    if (!isOpened) {
        return;
    }

    if (!m_selectedVersion) {
        m_dialog->setSuggestedPack();
        return;
    }

    m_ui->problemLabel->hide();

    // There isn't a selected version if the version list is empty
    if (m_selectedLoader == nullptr) {
        m_dialog->setSuggestedPack(m_selectedVersion->descriptor(), new VanillaCreationTask(m_selectedVersion));
    } else {
        QString suggestedName = QString("%1 %2").arg(m_selectedVersion->descriptor(), m_selectedLoader->name());
        if (m_selectedLoaderVersion == nullptr) {
            m_dialog->setSuggestedPack(suggestedName, nullptr);
            m_ui->problemLabel->show();
            m_ui->problemLabel->setText(u"<p style='color:red'>%1</p>"_s.arg(
                tr("The selected mod loader is not compatible with Minecraft %1!").arg(m_selectedVersion->descriptor())));
        } else {
            m_dialog->setSuggestedPack(
                suggestedName, new VanillaCreationTask(m_selectedVersion, m_selectedLoader->uid(), m_selectedLoaderVersion->descriptor()));
        }
    }
    m_dialog->setSuggestedIcon("default");
}

void CustomPage::setSelectedVersion(BaseVersion::Ptr version)
{
    m_selectedVersion = std::move(version);

    if (m_selectedVersion != nullptr) {
        syncLoader(m_selectedVersion->descriptor());
    }

    suggestCurrent();
}
