// SPDX-License-Identifier: GPL-3.0-only
/*
 *  ElyPrismLauncher - Minecraft Launcher
 *  Copyright (c) 2025 Octol1ttle <l1ttleofficial@outlook.com>
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

#include "ElyPatchTask.h"

#include <Application.h>
#include <meta/Index.h>
#include <minecraft/MinecraftInstance.h>
#include <minecraft/PackProfile.h>
#include <net/NetJob.h>

ElyPatchTask::ElyPatchTask(MinecraftInstance *inst, RuntimeContext &context, Net::Mode mode) : m_inst(inst), m_runtimeContext(context), m_netMode(mode)
{
}

void ElyPatchTask::executeTask()
{
    setStatus(tr("Preparing Ely.by patch..."));

    QString authlibVersion;
    const auto& libraries = m_inst->getPackProfile()->getProfile()->getLibraries();
    for (const auto& library : libraries) {
        if (const QString& artifact = library->artifactPrefix(); artifact == "com.mojang:authlib") {
            authlibVersion = library->version();
            break;
        }
    }

    if (!authlibVersion.isEmpty()) {
        resolveAuthlib(authlibVersion);
    } else {
        resolveAuthlibInjector();
    }
}

bool ElyPatchTask::canAbort() const
{
    return true;
}

bool ElyPatchTask::abort()
{
    return m_currentTask ? m_currentTask->abort() : true;
}

void ElyPatchTask::resolveAuthlib(QString version)
{
    setDetails("Resolving Ely.by Authlib");

    const auto metaVersionList = APPLICATION->metadataIndex()->get("by.ely.authlib");
    auto metaVersion = metaVersionList->getVersion(version);

    if (!metaVersion->isLoaded()) {
        m_currentTask = APPLICATION->metadataIndex()->loadVersion("by.ely.authlib", version, m_netMode);
        connect(m_currentTask.get(), &Task::succeeded, this, [this, metaVersion] {
            applyAuthlib(metaVersion);
        });
        connect(m_currentTask.get(), &Task::failed, this, [this](QString reason) {
            qWarning() << "Resolving Ely.by Authlib failed:" << reason;
            resolveAuthlibInjector();
        });
        connect(m_currentTask.get(), &Task::progress, this, &ElyPatchTask::setProgress);
        connect(m_currentTask.get(), &Task::stepProgress, this, &ElyPatchTask::propagateStepProgress);
        m_currentTask->start();
        return;
    }

    applyAuthlib(metaVersion);
}

void ElyPatchTask::resolveAuthlibInjector()
{
    setDetails(tr("Resolving authlib-injector"));

    const auto metaVersionList = APPLICATION->metadataIndex()->get("moe.yushi.authlibinjector");
    if (metaVersionList->status() == Meta::BaseEntity::LoadStatus::NotLoaded) {
        m_currentTask = metaVersionList->loadTask(m_netMode);
        connect(m_currentTask.get(), &Task::succeeded, this, [this] {
            resolveAuthlibInjector();
        });
        connect(m_currentTask.get(), &Task::failed, this, &ElyPatchTask::emitFailed);
        connect(m_currentTask.get(), &Task::progress, this, &ElyPatchTask::setProgress);
        connect(m_currentTask.get(), &Task::stepProgress, this, &ElyPatchTask::propagateStepProgress);
        m_currentTask->start();
        return;
    }

    Meta::Version::Ptr recommendedVersion = nullptr;
    for (int i = 0; i < metaVersionList->count(); ++i) {
        const auto version = metaVersionList->versions().at(i);
        if (version->isRecommended()) {
            recommendedVersion = version;
            break;
        }
    }
    if (!recommendedVersion) {
        emitFailed(tr("Couldn't get recommended authlib-injector version"));
        return;
    }

    if (!recommendedVersion->isLoaded()) {
        m_currentTask = APPLICATION->metadataIndex()->loadVersion("moe.yushi.authlibinjector", recommendedVersion->version(), m_netMode);
        connect(m_currentTask.get(), &Task::succeeded, this, [this, recommendedVersion] {
            applyMetaVersion(recommendedVersion);
        });
        connect(m_currentTask.get(), &Task::failed, this, &ElyPatchTask::emitFailed);
        connect(m_currentTask.get(), &Task::progress, this, &ElyPatchTask::setProgress);
        connect(m_currentTask.get(), &Task::stepProgress, this, &ElyPatchTask::propagateStepProgress);
        m_currentTask->start();
        return;
    }

    applyMetaVersion(recommendedVersion);
}

void ElyPatchTask::applyMetaVersion(Meta::Version::Ptr metaVersion)
{
    metaVersion->data()->applyTo(m_inst->getPackProfile()->getProfile().get(), m_runtimeContext);
    emitSucceeded();
}

void ElyPatchTask::applyAuthlib(Meta::Version::Ptr metaVersion)
{
    m_inst->getPackProfile()->getProfile()->removeLibrariesByPrefix("com.mojang:authlib");
    applyMetaVersion(metaVersion);
}
