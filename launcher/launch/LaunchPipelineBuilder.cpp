// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Colby Frison <colbyfrison100@gmail.com>
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
 */

#include "launch/LaunchPipelineBuilder.h"

#include "LaunchMode.h"
#include "MessageLevel.h"
#include "launch/LaunchTask.h"
#include "launch/TaskStepWrapper.h"
#include "launch/steps/CheckJava.h"
#include "launch/steps/LaunchCommand.h"
#include "launch/steps/LookupServerAddress.h"
#include "launch/steps/QuitAfterGameStop.h"
#include "launch/steps/TextPrint.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/MinecraftLoadAndCheck.h"
#include "minecraft/launch/AutoInstallJava.h"
#include "minecraft/launch/ClaimAccount.h"
#include "minecraft/launch/CreateGameFolders.h"
#include "minecraft/launch/EnsureAvailableMemory.h"
#include "minecraft/launch/EnsureOfflineLibraries.h"
#include "minecraft/launch/ExtractNatives.h"
#include "minecraft/launch/LauncherPartLaunch.h"
#include "minecraft/launch/ModMinecraftJar.h"
#include "minecraft/launch/PrintInstanceInfo.h"
#include "minecraft/launch/ReconstructAssets.h"
#include "minecraft/launch/ScanModFolders.h"
#include "minecraft/launch/VerifyJavaInstall.h"
#include "net/Mode.h"

#include <QLatin1String>

LaunchPipelineBuilder::LaunchPipelineBuilder(LaunchTask* task) : m_task(task) {}

LaunchPipelineBuilder& LaunchPipelineBuilder::setSession(AuthSessionPtr session)
{
    m_session = std::move(session);
    return *this;
}

LaunchPipelineBuilder& LaunchPipelineBuilder::setTargetToJoin(MinecraftTarget::Ptr target)
{
    m_targetToJoin = std::move(target);
    return *this;
}

LaunchPipelineBuilder& LaunchPipelineBuilder::setPreLoadCommand(const QString& command)
{
    m_preLoadCommand = command;
    return *this;
}

LaunchPipelineBuilder& LaunchPipelineBuilder::setPreLaunchCommand(const QString& command)
{
    m_preLaunchCommand = command;
    return *this;
}

LaunchPipelineBuilder& LaunchPipelineBuilder::setPostExitCommand(const QString& command)
{
    m_postExitCommand = command;
    return *this;
}

LaunchPipelineBuilder& LaunchPipelineBuilder::setQuitAfterGameStop(bool quit)
{
    m_quitAfterGameStop = quit;
    return *this;
}

bool LaunchPipelineBuilder::apply(const QString& recipeName)
{
    if (recipeName != QLatin1String(StandardMinecraftRecipe)) {
        return false;
    }
    if (!m_task || !m_task->instance() || !m_session) {
        return false;
    }

    // Phase order matches MinecraftInstance::createLaunchTask. Do not reorder these steps.
    appendPreMetadata();
    appendPreUpdate();
    appendPreLaunch();
    appendGame();
    appendPostExit();
    return true;
}

void LaunchPipelineBuilder::appendPreMetadata()
{
    auto* instance = m_task->instance();
    m_task->appendStep(makeShared<TextPrint>(m_task, "Minecraft folder is:\n  " + instance->gameRoot() + "\n", MessageLevel::Launcher));
    m_task->appendStep(makeShared<CreateGameFolders>(m_task));

    if (m_targetToJoin && m_targetToJoin->port == 25565) {
        auto step = makeShared<LookupServerAddress>(m_task);
        step->setLookupAddress(m_targetToJoin->address);
        step->setOutputAddressPtr(m_targetToJoin);
        m_task->appendStep(step);
    }

    if (!m_preLoadCommand.isEmpty()) {
        auto step = makeShared<LaunchCommand>(m_task, m_preLoadCommand, MinecraftInstance::tr("Pre-Load"));
        step->setWorkingDirectory(instance->gameRoot());
        m_task->appendStep(step);
    }
}

void LaunchPipelineBuilder::appendPreUpdate()
{
    auto* instance = m_task->instance();
    auto mode = m_session->launchMode != LaunchMode::Offline ? Net::Mode::Online : Net::Mode::Offline;
    m_task->appendStep(makeShared<TaskStepWrapper>(m_task, makeShared<MinecraftLoadAndCheck>(instance, mode)));
    m_task->appendStep(makeShared<AutoInstallJava>(m_task));
    m_task->appendStep(makeShared<CheckJava>(m_task));
    m_task->appendStep(makeShared<VerifyJavaInstall>(m_task));

    if (!m_preLaunchCommand.isEmpty()) {
        auto step = makeShared<LaunchCommand>(m_task, m_preLaunchCommand, MinecraftInstance::tr("Pre-Launch"));
        step->setWorkingDirectory(instance->gameRoot());
        m_task->appendStep(step);
    }
}

void LaunchPipelineBuilder::appendPreLaunch()
{
    auto* instance = m_task->instance();
    if (m_session->launchMode != LaunchMode::Offline) {
        m_task->appendStep(makeShared<ClaimAccount>(m_task, m_session));
        for (const auto& updateTask : instance->createUpdateTask()) {
            m_task->appendStep(makeShared<TaskStepWrapper>(m_task, updateTask));
        }
    } else {
        m_task->appendStep(makeShared<EnsureOfflineLibraries>(m_task, instance));
    }

    m_task->appendStep(makeShared<ModMinecraftJar>(m_task));
    m_task->appendStep(makeShared<ScanModFolders>(m_task));
    m_task->appendStep(makeShared<EnsureAvailableMemory>(m_task, instance));
    m_task->appendStep(makeShared<PrintInstanceInfo>(m_task, m_session, m_targetToJoin));
    m_task->appendStep(makeShared<ExtractNatives>(m_task));
    m_task->appendStep(makeShared<ReconstructAssets>(m_task));
}

void LaunchPipelineBuilder::appendGame()
{
    auto step = makeShared<LauncherPartLaunch>(m_task);
    step->setWorkingDirectory(m_task->instance()->gameRoot());
    step->setAuthSession(m_session);
    step->setTargetToJoin(m_targetToJoin);
    m_task->appendStep(step);
}

void LaunchPipelineBuilder::appendPostExit()
{
    if (!m_postExitCommand.isEmpty()) {
        auto step = makeShared<LaunchCommand>(m_task, m_postExitCommand, MinecraftInstance::tr("Post-Launch"));
        step->setWorkingDirectory(m_task->instance()->gameRoot());
        m_task->appendStep(step);
    }
    if (m_quitAfterGameStop) {
        m_task->appendStep(makeShared<QuitAfterGameStop>(m_task));
    }
}
