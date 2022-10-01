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

#include "LaunchController.h"
#include "minecraft/auth/AccountList.h"
#include "Application.h"

#include "ui/MainWindow.h"
#include "ui/InstanceWindow.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProfileSelectDialog.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/dialogs/EditAccountDialog.h"
#include "ui/dialogs/ProfileSetupDialog.h"

#include <QLineEdit>
#include <QInputDialog>
#include <QStringList>
#include <QHostInfo>
#include <QList>
#include <QHostAddress>
#include <QPushButton>

#include "BuildConfig.h"
#include "JavaCommon.h"
#include "tasks/Task.h"
#include "minecraft/auth/AccountTask.h"
#include "launch/steps/TextPrint.h"

LaunchController::LaunchController(QObject *parent) : Task(parent)
{
}

void LaunchController::executeTask()
{
    if (!m_instance)
    {
        emitFailed(tr("No instance specified!"));
        return;
    }

    if(!JavaCommon::checkJVMArgs(m_instance->settings()->get("JvmArgs").toString(), m_parentWidget)) {
        emitFailed(tr("Invalid Java arguments specified. Please fix this first."));
        return;
    }

    login();
}

void LaunchController::decideAccount()
{
    if(m_accountToUse) {
        return;
    }

    // Find an account to use.
    auto accounts = APPLICATION->accounts();
    if (accounts->count() <= 0)
    {
        // Tell the user they need to log in at least one account in order to play.
        auto reply = CustomMessageBox::selectable(
            m_parentWidget,
            tr("No Accounts"),
            tr("In order to play Minecraft, you must have at least one Microsoft "
               "account logged in. "
               "Would you like to open the account manager to add an account now?"),
            QMessageBox::Information,
            QMessageBox::Yes | QMessageBox::No
        )->exec();

        if (reply == QMessageBox::Yes)
        {
            // Open the account manager.
            APPLICATION->ShowGlobalSettings(m_parentWidget, "accounts");
        }
        else if (reply == QMessageBox::No)
        {
            // Do not open "profile select" dialog.
            return;
        }
    }

    m_accountToUse = accounts->defaultAccount();
    if (!m_accountToUse)
    {
        // If no default account is set, ask the user which one to use.
        ProfileSelectDialog selectDialog(
            tr("Which account would you like to use?"),
            ProfileSelectDialog::GlobalDefaultCheckbox,
            m_parentWidget
        );

        selectDialog.exec();

        // Launch the instance with the selected account.
        m_accountToUse = selectDialog.selectedAccount();

        // If the user said to use the account as default, do that.
        if (selectDialog.useAsGlobalDefault() && m_accountToUse) {
            accounts->setDefaultAccount(m_accountToUse);
        }
    }
}


void LaunchController::login() {
    decideAccount();

    // if no account is selected, we bail
    if (!m_accountToUse)
    {
        emitFailed(tr("No account selected for launch."));
        return;
    }

    // we loop until the user succeeds in logging in or gives up
    bool tryagain = true;
    unsigned int tries = 0;

    while (tryagain)
    {
        if (tries > 0 && tries % 3 == 0) {
            auto result = QMessageBox::question(
                m_parentWidget,
                tr("Continue launch?"),
                tr("It looks like we couldn't launch after %1 tries. Do you want to continue trying?")
                    .arg(tries)
            );

            if (result == QMessageBox::No) {
                emitAborted();
                return;
            }
        }
        tries++;
        m_session = std::make_shared<AuthSession>();
        m_session->wants_online = m_online;
        m_session->demo = m_demo;
        m_accountToUse->fillSession(m_session);

        // Launch immediately in true offline mode
        if(m_accountToUse->isOffline()) {
            launchInstance();
            return;
        }

        switch(m_accountToUse->accountState()) {
            case AccountState::Offline: {
                m_session->wants_online = false;
                // NOTE: fallthrough is intentional
            }
            case AccountState::Online: {
                if(!m_session->wants_online) {
                    // we ask the user for a player name
                    bool ok = false;

                    QString message = tr("Choose your offline mode player name.");
                    if(m_session->demo) {
                        message = tr("Choose your demo mode player name.");
                    }

                    QString lastOfflinePlayerName = APPLICATION->settings()->get("LastOfflinePlayerName").toString();
                    QString usedname = lastOfflinePlayerName.isEmpty() ? m_session->player_name : lastOfflinePlayerName;
                    QString name = QInputDialog::getText(
                        m_parentWidget,
                        tr("Player name"),
                        message,
                        QLineEdit::Normal,
                        usedname,
                        &ok
                    );
                    if (!ok)
                    {
                        tryagain = false;
                        break;
                    }
                    if (name.length())
                    {
                        usedname = name;
                        APPLICATION->settings()->set("LastOfflinePlayerName", usedname);
                    }
                    m_session->MakeOffline(usedname);
                    // offline flavored game from here :3
                }
                if(m_accountToUse->ownsMinecraft()) {
                    if(!m_accountToUse->hasProfile()) {
                        // Now handle setting up a profile name here...
                        ProfileSetupDialog dialog(m_accountToUse, m_parentWidget);
                        if (dialog.exec() == QDialog::Accepted)
                        {
                            tryagain = true;
                            continue;
                        }
                        else
                        {
                            emitFailed(tr("Received undetermined session status during login."));
                            return;
                        }
                    }
                    // we own Minecraft, there is a profile, it's all ready to go!
                    launchInstance();
                    return;
                }
                else {
                    // play demo ?
                    QMessageBox box(m_parentWidget);
                    box.setWindowTitle(tr("Play demo?"));
                    box.setText(tr("This account does not own Minecraft.\nYou need to purchase the game first to play it.\n\nDo you want to play the demo?"));
                    box.setIcon(QMessageBox::Warning);
                    auto demoButton = box.addButton(tr("Play Demo"), QMessageBox::ButtonRole::YesRole);
                    auto cancelButton = box.addButton(tr("Cancel"), QMessageBox::ButtonRole::NoRole);
                    box.setDefaultButton(cancelButton);

                    box.exec();
                    if(box.clickedButton() == demoButton) {
                        // play demo here
                        m_session->MakeDemo();
                        launchInstance();
                    }
                    else {
                        emitFailed(tr("Launch cancelled - account does not own Minecraft."));
                    }
                }
                return;
            }
            case AccountState::Errored:
                // This means some sort of soft error that we can fix with a refresh ... so let's refresh.
            case AccountState::Unchecked: {
                m_accountToUse->refresh();
                // NOTE: fallthrough intentional
            }
            case AccountState::Working: {
                // refresh is in progress, we need to wait for it to finish to proceed.
                ProgressDialog progDialog(m_parentWidget);
                if (m_online)
                {
                    progDialog.setSkipButton(true, tr("Play Offline"));
                }
                auto task = m_accountToUse->currentTask();
                progDialog.execWithTask(task.get());
                continue;
            }
            // FIXME: this is missing - the meaning is that the account is queued for refresh and we should wait for that
            /*
            case AccountState::Queued: {
                return;
            }
            */
            case AccountState::Expired: {
                auto errorString = tr("The account has expired and needs to be logged into manually again.");
                QMessageBox::warning(
                    m_parentWidget,
                    tr("Account refresh failed"),
                    errorString,
                    QMessageBox::StandardButton::Ok,
                    QMessageBox::StandardButton::Ok
                );
                emitFailed(errorString);
                return;
            }
            case AccountState::Disabled: {
                auto errorString = tr("The launcher's client identification has changed. Please remove this account and add it again.");
                QMessageBox::warning(
                        m_parentWidget,
                        tr("Client identification changed"),
                        errorString,
                        QMessageBox::StandardButton::Ok,
                        QMessageBox::StandardButton::Ok
                );
                emitFailed(errorString);
                return;
            }
            case AccountState::Gone: {
                auto errorString = tr("The account no longer exists on the servers. It may have been migrated, in which case please add the new account you migrated this one to.");
                QMessageBox::warning(
                    m_parentWidget,
                    tr("Account gone"),
                    errorString,
                    QMessageBox::StandardButton::Ok,
                    QMessageBox::StandardButton::Ok
                );
                emitFailed(errorString);
                return;
            }
        }
    }
    emitFailed(tr("Failed to launch."));
}

void LaunchController::launchInstance()
{
    Q_ASSERT_X(m_instance != NULL, "launchInstance", "instance is NULL");
    Q_ASSERT_X(m_session.get() != nullptr, "launchInstance", "session is NULL");

    if(!m_instance->reloadSettings())
    {
        QMessageBox::critical(m_parentWidget, tr("Error!"), tr("Couldn't load the instance profile."));
        emitFailed(tr("Couldn't load the instance profile."));
        return;
    }

    m_launcher = m_instance->createLaunchTask(m_session, m_serverToJoin);
    if (!m_launcher)
    {
        emitFailed(tr("Couldn't instantiate a launcher."));
        return;
    }

    auto console = qobject_cast<InstanceWindow *>(m_parentWidget);
    auto showConsole = m_instance->settings()->get("ShowConsole").toBool();
    if(!console && showConsole)
    {
        APPLICATION->showInstanceWindow(m_instance);
    }
    connect(m_launcher.get(), &LaunchTask::readyForLaunch, this, &LaunchController::readyForLaunch);
    connect(m_launcher.get(), &LaunchTask::succeeded, this, &LaunchController::onSucceeded);
    connect(m_launcher.get(), &LaunchTask::failed, this,  &LaunchController::onFailed);
    connect(m_launcher.get(), &LaunchTask::requestProgress, this, &LaunchController::onProgressRequested);

    // Prepend Online and Auth Status
    QString online_mode;
    if(m_session->wants_online) {
        online_mode = "online";

        // Prepend Server Status
        QStringList servers = {"authserver.mojang.com", "session.minecraft.net", "textures.minecraft.net", "api.mojang.com"};
        QString resolved_servers = "";
        QHostInfo host_info;

        for(QString server : servers) {
            host_info = QHostInfo::fromName(server);
            resolved_servers = resolved_servers + server + " resolves to:\n    [";
            if(!host_info.addresses().isEmpty()) {
                for(QHostAddress address : host_info.addresses()) {
                    resolved_servers = resolved_servers + address.toString();
                    if(!host_info.addresses().endsWith(address)) {
                        resolved_servers = resolved_servers + ", ";
                    }
                }
            } else {
                resolved_servers = resolved_servers + "N/A";
            }
            resolved_servers = resolved_servers + "]\n\n";
        }
        m_launcher->prependStep(new TextPrint(m_launcher.get(), resolved_servers, MessageLevel::Launcher));
    } else {
        online_mode = m_demo ? "demo" : "offline";
    }

    m_launcher->prependStep(new TextPrint(m_launcher.get(), "Launched instance in " + online_mode + " mode\n", MessageLevel::Launcher));

    // Prepend Version
    m_launcher->prependStep(new TextPrint(m_launcher.get(), BuildConfig.LAUNCHER_NAME + " version: " + BuildConfig.printableVersionString() + "\n\n", MessageLevel::Launcher));
    m_launcher->start();
}

void LaunchController::readyForLaunch()
{
    if (!m_profiler)
    {
        m_launcher->proceed();
        return;
    }

    QString error;
    if (!m_profiler->check(&error))
    {
        m_launcher->abort();
        QMessageBox::critical(m_parentWidget, tr("Error!"), tr("Couldn't start profiler: %1").arg(error));
        emitFailed("Profiler startup failed!");
        return;
    }
    BaseProfiler *profilerInstance = m_profiler->createProfiler(m_launcher->instance(), this);

    connect(profilerInstance, &BaseProfiler::readyToLaunch, [this](const QString & message)
    {
        QMessageBox msg;
        msg.setText(tr("The game launch is delayed until you press the "
                        "button. This is the right time to setup the profiler, as the "
                        "profiler server is running now.\n\n%1").arg(message));
        msg.setWindowTitle(tr("Waiting."));
        msg.setIcon(QMessageBox::Information);
        msg.addButton(tr("Launch"), QMessageBox::AcceptRole);
        msg.setModal(true);
        msg.exec();
        m_launcher->proceed();
    });
    connect(profilerInstance, &BaseProfiler::abortLaunch, [this](const QString & message)
    {
        QMessageBox msg;
        msg.setText(tr("Couldn't start the profiler: %1").arg(message));
        msg.setWindowTitle(tr("Error"));
        msg.setIcon(QMessageBox::Critical);
        msg.addButton(QMessageBox::Ok);
        msg.setModal(true);
        msg.exec();
        m_launcher->abort();
        emitFailed("Profiler startup failed!");
    });
    profilerInstance->beginProfiling(m_launcher);
}

void LaunchController::onSucceeded()
{
    emitSucceeded();
}

void LaunchController::onFailed(QString reason)
{
    if(m_instance->settings()->get("ShowConsoleOnError").toBool())
    {
        APPLICATION->showInstanceWindow(m_instance, "console");
    }
    emitFailed(reason);
}

void LaunchController::onProgressRequested(Task* task)
{
    ProgressDialog progDialog(m_parentWidget);
    progDialog.setSkipButton(true, tr("Abort"));
    m_launcher->proceed();
    progDialog.execWithTask(task);
}

bool LaunchController::abort()
{
    if(!m_launcher)
    {
        return true;
    }
    if(!m_launcher->canAbort())
    {
        return false;
    }
    auto response = CustomMessageBox::selectable(
            m_parentWidget, tr("Kill Minecraft?"),
            tr("This can cause the instance to get corrupted and should only be used if Minecraft "
            "is frozen for some reason"),
            QMessageBox::Question, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)->exec();
    if (response == QMessageBox::Yes)
    {
        return m_launcher->abort();
    }
    return false;
}
