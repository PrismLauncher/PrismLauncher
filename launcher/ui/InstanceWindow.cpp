// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

#include "InstanceWindow.h"
#include "Application.h"

#include <qlayoutitem.h>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>

#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/widgets/PageContainer.h"

#include "InstancePageProvider.h"

#include "icons/IconList.h"

InstanceWindow::InstanceWindow(InstancePtr instance, QWidget* parent) : QMainWindow(parent), m_instance(instance)
{
    setAttribute(Qt::WA_DeleteOnClose);

    auto icon = APPLICATION->icons()->getIcon(m_instance->iconKey());
    QString windowTitle = tr("Console window for ") + m_instance->name();

    // Set window properties
    {
        setWindowIcon(icon);
        setWindowTitle(windowTitle);
    }

    // Add page container
    {
        auto provider = std::make_shared<InstancePageProvider>(m_instance);
        m_container = new PageContainer(provider.get(), "console", this);
        m_container->setParentContainer(this);
        setCentralWidget(m_container);
        setContentsMargins(0, 0, 0, 0);
    }

    // Add custom buttons to the page container layout.
    {
        auto horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setContentsMargins(6, -1, 6, -1);

        auto btnHelp = new QPushButton();
        btnHelp->setText(tr("Help"));
        horizontalLayout->addWidget(btnHelp);
        connect(btnHelp, SIGNAL(clicked(bool)), m_container, SLOT(help()));

        auto spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        horizontalLayout->addSpacerItem(spacer);

        m_killButton = new QPushButton();
        horizontalLayout->addWidget(m_killButton);
        connect(m_killButton, SIGNAL(clicked(bool)), SLOT(on_btnKillMinecraft_clicked()));

        m_launchOfflineButton = new QPushButton();
        horizontalLayout->addWidget(m_launchOfflineButton);
        m_launchOfflineButton->setText(tr("Launch Offline"));

        m_launchDemoButton = new QPushButton();
        horizontalLayout->addWidget(m_launchDemoButton);
        m_launchDemoButton->setText(tr("Launch Demo"));

        updateLaunchButtons();
        connect(m_launchOfflineButton, SIGNAL(clicked(bool)), SLOT(on_btnLaunchMinecraftOffline_clicked()));
        connect(m_launchDemoButton, SIGNAL(clicked(bool)), SLOT(on_btnLaunchMinecraftDemo_clicked()));

        m_closeButton = new QPushButton();
        m_closeButton->setText(tr("Close"));
        horizontalLayout->addWidget(m_closeButton);
        connect(m_closeButton, SIGNAL(clicked(bool)), SLOT(on_closeButton_clicked()));

        m_container->addButtons(horizontalLayout);
    }

    // restore window state
    {
        auto base64State = APPLICATION->settings()->get("ConsoleWindowState").toByteArray();
        restoreState(QByteArray::fromBase64(base64State));
        auto base64Geometry = APPLICATION->settings()->get("ConsoleWindowGeometry").toByteArray();
        restoreGeometry(QByteArray::fromBase64(base64Geometry));
    }

    // set up instance and launch process recognition
    {
        auto launchTask = m_instance->getLaunchTask();
        instanceLaunchTaskChanged(launchTask);
        connect(m_instance.get(), &BaseInstance::launchTaskChanged, this, &InstanceWindow::instanceLaunchTaskChanged);
        connect(m_instance.get(), &BaseInstance::runningStatusChanged, this, &InstanceWindow::runningStateChanged);
    }

    // set up instance destruction detection
    {
        connect(m_instance.get(), &BaseInstance::statusChanged, this, &InstanceWindow::on_instanceStatusChanged);
    }

    // add ourself as the modpack page's instance window
    {
        static_cast<ManagedPackPage*>(m_container->getPage("managed_pack"))->setInstanceWindow(this);
    }

    show();
}

void InstanceWindow::on_instanceStatusChanged(BaseInstance::Status, BaseInstance::Status newStatus)
{
    if (newStatus == BaseInstance::Status::Gone) {
        m_doNotSave = true;
        close();
    }
}

void InstanceWindow::updateLaunchButtons()
{
    if (m_instance->isRunning()) {
        m_launchOfflineButton->setEnabled(false);
        m_launchDemoButton->setEnabled(false);
        m_killButton->setText(tr("Kill"));
        m_killButton->setObjectName("killButton");
        m_killButton->setToolTip(tr("Kill the running instance"));
    } else if (!m_instance->canLaunch()) {
        m_launchOfflineButton->setEnabled(false);
        m_launchDemoButton->setEnabled(false);
        m_killButton->setText(tr("Launch"));
        m_killButton->setObjectName("launchButton");
        m_killButton->setToolTip(tr("Launch the instance"));
        m_killButton->setEnabled(false);
    } else {
        m_launchOfflineButton->setEnabled(true);

        // Disable demo-mode if not available.
        auto instance = dynamic_cast<MinecraftInstance*>(m_instance.get());
        if (instance) {
            m_launchDemoButton->setEnabled(instance->supportsDemo());
        }

        m_killButton->setText(tr("Launch"));
        m_killButton->setObjectName("launchButton");
        m_killButton->setToolTip(tr("Launch the instance"));
    }
    // NOTE: this is a hack to force the button to recalculate its style
    m_killButton->setStyleSheet("/* */");
    m_killButton->setStyleSheet(QString());
}

void InstanceWindow::on_btnLaunchMinecraftOffline_clicked()
{
    APPLICATION->launch(m_instance, false, false, nullptr);
}

void InstanceWindow::on_btnLaunchMinecraftDemo_clicked()
{
    APPLICATION->launch(m_instance, false, true, nullptr);
}

void InstanceWindow::instanceLaunchTaskChanged(shared_qobject_ptr<LaunchTask> proc)
{
    m_proc = proc;
}

void InstanceWindow::runningStateChanged(bool running)
{
    updateLaunchButtons();
    m_container->refreshContainer();
    if (running) {
        selectPage("log");
    }
}

void InstanceWindow::on_closeButton_clicked()
{
    close();
}

void InstanceWindow::closeEvent(QCloseEvent* event)
{
    bool proceed = true;
    if (!m_doNotSave) {
        proceed &= m_container->prepareToClose();
    }

    if (!proceed) {
        return;
    }

    APPLICATION->settings()->set("ConsoleWindowState", saveState().toBase64());
    APPLICATION->settings()->set("ConsoleWindowGeometry", saveGeometry().toBase64());
    emit isClosing();
    event->accept();
}

bool InstanceWindow::saveAll()
{
    return m_container->saveAll();
}

void InstanceWindow::on_btnKillMinecraft_clicked()
{
    if (m_instance->isRunning()) {
        APPLICATION->kill(m_instance);
    } else {
        APPLICATION->launch(m_instance, true, false, nullptr);
    }
}

QString InstanceWindow::instanceId()
{
    return m_instance->id();
}

bool InstanceWindow::selectPage(QString pageId)
{
    return m_container->selectPage(pageId);
}

void InstanceWindow::refreshContainer()
{
    m_container->refreshContainer();
}

InstanceWindow::~InstanceWindow() {}

bool InstanceWindow::requestClose()
{
    if (m_container->prepareToClose()) {
        close();
        return true;
    }
    return false;
}
