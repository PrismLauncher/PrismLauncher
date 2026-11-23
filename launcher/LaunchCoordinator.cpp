// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Prism Launcher Contributors
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

#include "LaunchCoordinator.h"

#include <QDebug>
#include <QMetaObject>

#include "BaseInstance.h"
#include "LaunchController.h"
#include "minecraft/MinecraftInstance.h"
#include "services/LauncherWindows.h"
#include "tasks/Task.h"

LaunchCoordinator::LaunchCoordinator(LaunchHost* host, QObject* parent) : QObject(parent), m_host(host) {}

bool LaunchCoordinator::launch(MinecraftInstance* instance,
                               LaunchMode mode,
                               MinecraftTarget::Ptr targetToJoin,
                               MinecraftAccountPtr accountToUse,
                               const QString& offlineName)
{
    if (m_host == nullptr || instance == nullptr) {
        return false;
    }
    if (m_host->isUpdateRunning()) {
        qDebug() << "Cannot launch instances while an update is running. Please try again when updates are completed.";
    } else if (instance->canLaunch()) {
        if (!m_host->saveInstanceWindow(instance->id())) {
            return false;
        }
        auto controller = std::make_unique<LaunchController>();
        controller->setInstance(instance);
        controller->setLaunchMode(mode);
        controller->setProfiler(m_host->profiler(instance->settings()->get("Profiler").toString()));
        controller->setTargetToJoin(std::move(targetToJoin));
        controller->setAccountToUse(std::move(accountToUse));
        controller->setOfflineName(offlineName);
        if (QWidget* window = m_host->instanceWindow(instance->id())) {
            controller->setParentWidget(window);
        } else if (QWidget* mainWindow = m_host->mainWindow()) {
            controller->setParentWidget(mainWindow);
        }
        auto* raw = controller.get();
        m_host->adoptController(instance->id(), std::move(controller));
        m_host->watch(raw);
        m_host->addRunningInstance();
        QMetaObject::invokeMethod(raw, &Task::start, Qt::QueuedConnection);
        return true;
    } else if (instance->isRunning()) {
        if (auto* windows = m_host->windows()) {
            windows->showInstanceWindow(instance, "console");
        }
        return true;
    } else if (instance->canEdit()) {
        if (auto* windows = m_host->windows()) {
            windows->showInstanceWindow(instance);
        }
        return true;
    }
    return false;
}

bool LaunchCoordinator::kill(BaseInstance* instance)
{
    if (m_host == nullptr || instance == nullptr) {
        return false;
    }
    if (!instance->isRunning()) {
        qWarning() << "Attempted to kill instance" << instance->id() << ", which isn't running.";
        return false;
    }
    if (auto* controller = m_host->controller(instance->id())) {
        return controller->abort();
    }
    return true;
}

const QMap<QString, std::shared_ptr<BaseProfilerFactory>>& LaunchCoordinator::profilers() const
{
    return m_host->profilers();
}
