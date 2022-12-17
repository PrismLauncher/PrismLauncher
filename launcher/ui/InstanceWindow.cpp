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

#include "InstanceWindow.h"
#include "Application.h"

#include <QScrollBar>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <qlayoutitem.h>
#include <QCloseEvent>

#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/widgets/PageContainer.h"

#include "InstancePageProvider.h"

#include "icons/IconList.h"

InstanceWindow::InstanceWindow(InstancePtr instance, QWidget *parent)
    : QMainWindow(parent), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance(instance)
{
    setAttribute(Qt::WA_DeleteOnClose);

    auto icon = APPLICATION->icons()->getIcon(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance->iconKey());
    QString windowTitle = tr("Console window for ") + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance->name();

    // Set window properties
    {
        setWindowIcon(icon);
        setWindowTitle(windowTitle);
    }

    // Add page container
    {
        auto provider = std::make_shared<InstancePageProvider>(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_container = new PageContainer(provider.get(), "console", this);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_container->setParentContainer(this);
        setCentralWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_container);
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
        connect(btnHelp, SIGNAL(clicked(bool)), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_container, SLOT(help()));

        auto spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        horizontalLayout->addSpacerItem(spacer);

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_killButton = new QPushButton();
        horizontalLayout->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_killButton);
        connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_killButton, SIGNAL(clicked(bool)), SLOT(on_btnKillMinecraft_clicked()));

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_launchOfflineButton = new QPushButton();
        horizontalLayout->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_launchOfflineButton);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_launchOfflineButton->setText(tr("Launch Offline"));

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_launchDemoButton = new QPushButton();
        horizontalLayout->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_launchDemoButton);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_launchDemoButton->setText(tr("Launch Demo"));

        updateLaunchButtons();
        connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_launchOfflineButton, SIGNAL(clicked(bool)), SLOT(on_btnLaunchMinecraftOffline_clicked()));
        connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_launchDemoButton, SIGNAL(clicked(bool)), SLOT(on_btnLaunchMinecraftDemo_clicked()));

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_closeButton = new QPushButton();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_closeButton->setText(tr("Close"));
        horizontalLayout->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_closeButton);
        connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_closeButton, SIGNAL(clicked(bool)), SLOT(on_closeButton_clicked()));

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_container->addButtons(horizontalLayout);
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
        auto launchTask = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance->getLaunchTask();
        instanceLaunchTaskChanged(launchTask);
        connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance.get(), &BaseInstance::launchTaskChanged, this, &InstanceWindow::instanceLaunchTaskChanged);
        connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance.get(), &BaseInstance::runningStatusChanged, this, &InstanceWindow::runningStateChanged);
    }

    // set up instance destruction detection
    {
        connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance.get(), &BaseInstance::statusChanged, this, &InstanceWindow::on_instanceStatusChanged);
    }

    // add ourself as the modpack page's instance window
    {
        static_cast<ManagedPackPage*>(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_container->getPage("managed_pack"))->setInstanceWindow(this);
    }

    show();
}

void InstanceWindow::on_instanceStatusChanged(BaseInstance::Status, BaseInstance::Status newStatus)
{
    if(newStatus == BaseInstance::Status::Gone)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_doNotSave = true;
        close();
    }
}

void InstanceWindow::updateLaunchButtons()
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance->isRunning())
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_launchOfflineButton->setEnabled(false);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_launchDemoButton->setEnabled(false);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_killButton->setText(tr("Kill"));
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_killButton->setObjectName("killButton");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_killButton->setToolTip(tr("Kill the running instance"));
    }
    else if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance->canLaunch())
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_launchOfflineButton->setEnabled(false);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_launchDemoButton->setEnabled(false);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_killButton->setText(tr("Launch"));
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_killButton->setObjectName("launchButton");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_killButton->setToolTip(tr("Launch the instance"));
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_killButton->setEnabled(false);
    }
    else
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_launchOfflineButton->setEnabled(true);

        // Disable demo-mode if not available.
        auto instance = dynamic_cast<MinecraftInstance*>(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance.get());
        if (instance) {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_launchDemoButton->setEnabled(instance->supportsDemo());
        }

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_killButton->setText(tr("Launch"));
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_killButton->setObjectName("launchButton");
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_killButton->setToolTip(tr("Launch the instance"));
    }
    // NOTE: this is a hack to force the button to recalculate its style
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_killButton->setStyleSheet("/* */");
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_killButton->setStyleSheet(QString());
}

void InstanceWindow::on_btnLaunchMinecraftOffline_clicked()
{
    APPLICATION->launch(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance, false, false, nullptr);
}

void InstanceWindow::on_btnLaunchMinecraftDemo_clicked()
{
    APPLICATION->launch(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance, false, true, nullptr);
}

void InstanceWindow::instanceLaunchTaskChanged(shared_qobject_ptr<LaunchTask> proc)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proc = proc;
}

void InstanceWindow::runningStateChanged(bool running)
{
    updateLaunchButtons();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_container->refreshContainer();
    if(running) {
        selectPage("log");
    }
}

void InstanceWindow::on_closeButton_clicked()
{
    close();
}

void InstanceWindow::closeEvent(QCloseEvent *event)
{
    bool proceed = true;
    if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_doNotSave)
    {
        proceed &= hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_container->prepareToClose();
    }

    if(!proceed)
    {
        return;
    }

    APPLICATION->settings()->set("ConsoleWindowState", saveState().toBase64());
    APPLICATION->settings()->set("ConsoleWindowGeometry", saveGeometry().toBase64());
    emit isClosing();
    event->accept();
}

bool InstanceWindow::saveAll()
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_container->saveAll();
}

void InstanceWindow::on_btnKillMinecraft_clicked()
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance->isRunning())
    {
        APPLICATION->kill(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance);
    }
    else
    {
        APPLICATION->launch(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance, true, false, nullptr);
    }
}

QString InstanceWindow::instanceId()
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance->id();
}

bool InstanceWindow::selectPage(QString pageId)
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_container->selectPage(pageId);
}

void InstanceWindow::refreshContainer()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_container->refreshContainer();
}

InstanceWindow::~InstanceWindow()
{
}

bool InstanceWindow::requestClose()
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_container->prepareToClose())
    {
        close();
        return true;
    }
    return false;
}
