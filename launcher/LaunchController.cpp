// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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
#include "Application.h"
#include "launch/steps/PrintServers.h"
#include "minecraft/auth/AccountData.h"
#include "minecraft/auth/AccountList.h"

#include "ui/InstanceWindow.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/MSALoginDialog.h"
#include "ui/dialogs/ProfileSelectDialog.h"
#include "ui/dialogs/ProfileSetupDialog.h"
#include "ui/dialogs/ProgressDialog.h"

#include <QInputDialog>
#include <QList>
#include <QPushButton>
#include <utility>

#include "BuildConfig.h"
#include "JavaCommon.h"
#include "launch/steps/TextPrint.h"
#include "tasks/Task.h"
#include "ui/dialogs/ChooseOfflineNameDialog.h"

LaunchController::LaunchController() = default;

void LaunchController::executeTask()
{
    if (!m_instance) {
        emitFailed(tr("No instance specified!"));
        return;
    }

    if (!JavaCommon::checkJVMArgs(m_instance->settings()->get("JvmArgs").toString(), m_parentWidget)) {
        emitFailed(tr("Invalid Java arguments specified. Please fix this first."));
        return;
    }

    login();
}

void LaunchController::decideAccount()
{
    if (m_accountToUse) {
        return;
    }

    // Select the account to use. If the instance has a specific account set, that will be used. Otherwise, the default account will be used
    auto* accounts = APPLICATION->accounts();
    const auto instanceAccountId = m_instance->settings()->get("InstanceAccountId").toString();
    const auto instanceAccountIndex = accounts->findAccountByProfileId(instanceAccountId);
    if (instanceAccountIndex == -1 || instanceAccountId.isEmpty()) {
        m_accountToUse = accounts->defaultAccount();
    } else {
        m_accountToUse = accounts->at(instanceAccountIndex);
    }

    if (!accounts->anyAccountIsValid()) {
        // Tell the user they need to log in at least one account in order to play.
        auto reply = CustomMessageBox::selectable(m_parentWidget, tr("No Accounts"),
                                                  tr("In order to play Minecraft, you must have at least one Microsoft "
                                                     "account which owns Minecraft logged in. "
                                                     "Would you like to open the account manager to add an account now?"),
                                                  QMessageBox::Information, QMessageBox::Yes | QMessageBox::No)
                         ->exec();

        if (reply == QMessageBox::Yes) {
            // Open the account manager.
            APPLICATION->ShowGlobalSettings(m_parentWidget, "accounts");
        } else if (reply == QMessageBox::No) {
            // Do not open "profile select" dialog.
            return;
        }
    }

    if (!m_accountToUse) {
        // If no default account is set, ask the user which one to use.
        ProfileSelectDialog selectDialog(tr("Which account would you like to use?"), ProfileSelectDialog::GlobalDefaultCheckbox,
                                         m_parentWidget);

        selectDialog.exec();

        // Launch the instance with the selected account.
        m_accountToUse = selectDialog.selectedAccount();

        // If the user said to use the account as default, do that.
        if (selectDialog.useAsGlobalDefault() && m_accountToUse) {
            accounts->setDefaultAccount(m_accountToUse);
        }
    }
}

LaunchDecision LaunchController::decideLaunchMode()
{
    if (!m_accountToUse || m_wantedLaunchMode == LaunchMode::Demo) {
        m_actualLaunchMode = LaunchMode::Demo;
        return LaunchDecision::Continue;
    }

    if (m_wantedLaunchMode == LaunchMode::Normal) {
        if (m_accountToUse->shouldRefresh() || m_accountToUse->accountState() == AccountState::Offline) {
            // Force account refresh on the account used to launch the instance updating the AccountState
            // only on first try and if it is not meant to be offline
            m_accountToUse->refresh();
        }
    }

    const auto* accounts = APPLICATION->accounts();
    MinecraftAccountPtr accountToCheck = nullptr;

    if (m_accountToUse->accountType() != AccountType::Offline) {
        accountToCheck = m_accountToUse->ownsMinecraft() ? m_accountToUse : nullptr;
    } else if (const auto defaultAccount = accounts->defaultAccount(); defaultAccount && defaultAccount->ownsMinecraft()) {
        accountToCheck = defaultAccount;
    } else {
        for (int i = 0; i < accounts->count(); i++) {
            if (const auto account = accounts->at(i); account->ownsMinecraft()) {
                accountToCheck = account;
                break;
            }
        }
    }

    if (!accountToCheck) {
        m_actualLaunchMode = LaunchMode::Demo;
        return LaunchDecision::Continue;
    }

    auto state = accountToCheck->accountState();
    if (state == AccountState::Unchecked || state == AccountState::Errored) {
        accountToCheck->refresh();
        state = AccountState::Working;
    }

    if (state == AccountState::Working) {
        // refresh is in progress, we need to wait for it to finish to proceed.
        ProgressDialog progDialog(m_parentWidget);
        progDialog.setSkipButton(true, tr("Abort"));

        // TODO: this relies on tasks' synchronous signal dispatching nature
        // TODO: meaning currentTask can't complete and become null while this code is running
        // TODO: this code will produce a race condition when tasks become fully async
        auto task = accountToCheck->currentTask();
        progDialog.execWithTask(task.get());

        if (task->getState() == State::AbortedByUser) {
            return LaunchDecision::Abort;
        }

        state = accountToCheck->accountState();
    }

    QString reauthReason;
    switch (state) {
        case AccountState::Errored:
            reauthReason = tr("An error occurred while refreshing '%1'").arg(accountToCheck->profileName());
            break;
        case AccountState::Expired:
            reauthReason = tr("'%1' has expired and needs to be reauthenticated").arg(accountToCheck->profileName());
            break;
        case AccountState::Disabled:
            reauthReason = tr("The launcher's client identification has changed");
            break;
        case AccountState::Gone:
            reauthReason = tr("'%1' no longer exists on the servers").arg(accountToCheck->profileName());
            break;
        default:
            m_actualLaunchMode =
                state == AccountState::Online && m_wantedLaunchMode == LaunchMode::Normal ? LaunchMode::Normal : LaunchMode::Offline;
            return LaunchDecision::Continue;  // All good to go
    }

    if (reauthenticateAccount(accountToCheck, reauthReason)) {
        return LaunchDecision::Undecided;
    }

    return LaunchDecision::Abort;
}

bool LaunchController::askPlayDemo() const
{
    QMessageBox box(m_parentWidget);
    box.setWindowTitle(tr("Play demo?"));
    QString text = m_accountToUse
                       ? tr("This account does not own Minecraft.\nYou need to purchase the game first to play the full version.")
                       : tr("No account was selected for launch.");
    text += tr("\n\nDo you want to play the demo?");
    box.setText(text);
    box.setIcon(QMessageBox::Warning);
    const auto* demoButton = box.addButton(tr("Play Demo"), QMessageBox::ButtonRole::YesRole);
    auto* cancelButton = box.addButton(tr("Cancel"), QMessageBox::ButtonRole::NoRole);
    box.setDefaultButton(cancelButton);

    box.exec();
    return box.clickedButton() == demoButton;
}

QString LaunchController::askOfflineName(const QString& playerName, bool* ok) const
{
    if (ok != nullptr) {
        *ok = false;
    }

    QString message;
    switch (m_actualLaunchMode) {
        case LaunchMode::Normal:
            Q_ASSERT(false);
            return "";
        case LaunchMode::Demo:
            message = tr("Choose your demo mode player name");
            break;
        case LaunchMode::Offline:
            if (m_wantedLaunchMode == LaunchMode::Normal) {
                message = tr("You are not connected to the Internet, launching in offline mode\n\n");
            }
            message += tr("Choose your offline mode player name");
            break;
    }

    const QString lastOfflinePlayerName = APPLICATION->settings()->get("LastOfflinePlayerName").toString();
    QString usedname = lastOfflinePlayerName.isEmpty() ? playerName : lastOfflinePlayerName;

    ChooseOfflineNameDialog dialog(message, m_parentWidget);
    dialog.setWindowTitle(tr("Player name"));
    dialog.setUsername(usedname);
    if (dialog.exec() != QDialog::Accepted) {
        return {};
    }

    usedname = dialog.getUsername();
    APPLICATION->settings()->set("LastOfflinePlayerName", usedname);

    if (ok != nullptr) {
        *ok = true;
    }
    return usedname;
}

void LaunchController::login()
{
    decideAccount();

    LaunchDecision decision = decideLaunchMode();
    while (decision == LaunchDecision::Undecided) {
        decision = decideLaunchMode();
    }
    if (decision == LaunchDecision::Abort) {
        emitAborted();
        return;
    }

    if (m_actualLaunchMode == LaunchMode::Demo) {
        if (m_wantedLaunchMode == LaunchMode::Demo || askPlayDemo()) {
            bool ok = false;
            auto name = askOfflineName("Player", &ok);
            if (ok) {
                m_session = std::make_shared<AuthSession>();
                m_session->MakeDemo(name, MinecraftAccount::uuidFromUsername(name).toString(QUuid::Id128));
                launchInstance();
                return;
            }
        }

        emitFailed(tr("No account selected for launch"));
        return;
    }

    m_session = std::make_shared<AuthSession>();
    m_session->launchMode = m_actualLaunchMode;
    m_accountToUse->fillSession(m_session);

    if (m_accountToUse->accountType() != AccountType::Offline) {
        if (m_actualLaunchMode == LaunchMode::Normal && !m_accountToUse->hasProfile()) {
            // Now handle setting up a profile name here...
            if (ProfileSetupDialog dialog(m_accountToUse, m_parentWidget); dialog.exec() != QDialog::Accepted) {
                emitAborted();
                return;
            }
        }

        if (m_actualLaunchMode == LaunchMode::Offline && m_accountToUse->accountType() != AccountType::Offline) {
            bool ok = false;
            QString name = m_offlineName;
            if (name.isEmpty()) {
                name = askOfflineName(m_session->player_name, &ok);
                if (!ok) {
                    emitAborted();
                    return;
                }
            }
            m_session->MakeOffline(name);
        }
    }

    launchInstance();
}

bool LaunchController::reauthenticateAccount(const MinecraftAccountPtr& account, const QString& reason)
{
    auto button = QMessageBox::warning(
        m_parentWidget, tr("Account refresh failed"), tr("%1. Do you want to reauthenticate this account?").arg(reason),
        QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes);
    if (button == QMessageBox::StandardButton::Yes) {
        auto* accounts = APPLICATION->accounts();
        const bool isDefault = accounts->defaultAccount() == account;
        accounts->removeAccount(accounts->index(accounts->findAccountByProfileId(account->profileId())));
        if (account->accountType() == AccountType::MSA) {
            auto newAccount = MSALoginDialog::newAccount(m_parentWidget);

            if (newAccount != nullptr) {
                accounts->addAccount(newAccount);

                if (isDefault) {
                    accounts->setDefaultAccount(newAccount);
                }

                if (m_accountToUse == account) {
                    m_accountToUse = nullptr;
                    decideAccount();
                }
                return true;
            }
        }
    }

    return false;
}

void LaunchController::launchInstance()
{
    Q_ASSERT(m_instance != nullptr);
    Q_ASSERT(m_session.get() != nullptr);

    if (!m_instance->reloadSettings()) {
        QMessageBox::critical(m_parentWidget, tr("Error!"), tr("Couldn't load the instance profile."));
        emitFailed(tr("Couldn't load the instance profile."));
        return;
    }

    m_launcher = m_instance->createLaunchTask(m_session, m_targetToJoin);
    if (!m_launcher) {
        emitFailed(tr("Couldn't instantiate a launcher."));
        return;
    }

    const auto* console = qobject_cast<InstanceWindow*>(m_parentWidget);
    const auto showConsole = m_instance->settings()->get("ShowConsole").toBool();
    if (!console && showConsole) {
        APPLICATION->showInstanceWindow(m_instance);
    }
    connect(m_launcher, &LaunchTask::readyForLaunch, this, &LaunchController::readyForLaunch);
    connect(m_launcher, &LaunchTask::succeeded, this, &LaunchController::onSucceeded);
    connect(m_launcher, &LaunchTask::failed, this, &LaunchController::onFailed);
    connect(m_launcher, &LaunchTask::requestProgress, this, &LaunchController::onProgressRequested);

    // Prepend Online and Auth Status
    QString online_mode;
    if (m_actualLaunchMode == LaunchMode::Normal) {
        online_mode = "online";

        // Prepend Server Status
        const QStringList servers = { "login.microsoftonline.com", "session.minecraft.net", "textures.minecraft.net", "api.mojang.com" };

        m_launcher->prependStep(makeShared<PrintServers>(m_launcher, servers));
    } else {
        online_mode = m_actualLaunchMode == LaunchMode::Demo ? "demo" : "offline";
    }

    m_launcher->prependStep(makeShared<TextPrint>(m_launcher, "Launched instance in " + online_mode + " mode\n", MessageLevel::Launcher));

    // Prepend Version
    {
        auto versionString = QString("%1 version: %2 (%3)")
                                 .arg(BuildConfig.LAUNCHER_DISPLAYNAME, BuildConfig.printableVersionString(), BuildConfig.BUILD_PLATFORM);
        m_launcher->prependStep(makeShared<TextPrint>(m_launcher, versionString + "\n", MessageLevel::Launcher));
    }
    m_launcher->start();
}

void LaunchController::readyForLaunch()
{
    if (!m_profiler) {
        m_launcher->proceed();
        return;
    }

    QString error;
    if (!m_profiler->check(&error)) {
        m_launcher->abort();
        emitFailed("Profiler startup failed!");
        QMessageBox::critical(m_parentWidget, tr("Error!"), tr("Profiler check for %1 failed: %2").arg(m_profiler->name(), error));
        return;
    }
    BaseProfiler* profilerInstance = m_profiler->createProfiler(m_launcher->instance(), this);

    connect(profilerInstance, &BaseProfiler::readyToLaunch, [this](const QString& message) {
        QMessageBox msg(m_parentWidget);
        msg.setText(tr("The game launch is delayed until you press the "
                       "button. This is the right time to setup the profiler, as the "
                       "profiler server is running now.\n\n%1")
                        .arg(message));
        msg.setWindowTitle(tr("Waiting."));
        msg.setIcon(QMessageBox::Information);
        msg.addButton(tr("&Launch"), QMessageBox::AcceptRole);
        msg.exec();
        m_launcher->proceed();
    });
    connect(profilerInstance, &BaseProfiler::abortLaunch, [this](const QString& message) {
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
    if (m_instance->settings()->get("ShowConsoleOnError").toBool()) {
        APPLICATION->showInstanceWindow(m_instance, "console");
    }
    emitFailed(std::move(reason));
}

void LaunchController::onProgressRequested(Task* task) const
{
    ProgressDialog progDialog(m_parentWidget);
    progDialog.setSkipButton(true, tr("Abort"));
    m_launcher->proceed();
    progDialog.execWithTask(task);
}

bool LaunchController::abort()
{
    if (!m_launcher) {
        return true;
    }
    if (!m_launcher->canAbort()) {
        return false;
    }
    auto response = CustomMessageBox::selectable(m_parentWidget, tr("Kill Minecraft?"),
                                                 tr("This can cause the instance to get corrupted and should only be used if Minecraft "
                                                    "is frozen for some reason"),
                                                 QMessageBox::Question, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)
                        ->exec();
    if (response == QMessageBox::Yes) {
        return m_launcher->abort();
    }
    return false;
}
