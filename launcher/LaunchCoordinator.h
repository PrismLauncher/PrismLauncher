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

#pragma once

#include <QMap>
#include <QObject>
#include <memory>

#include "LaunchMode.h"
#include "QObjectPtr.h"
#include "minecraft/auth/MinecraftAccount.h"
#include "minecraft/launch/MinecraftTarget.h"
#include "tools/BaseProfiler.h"

class BaseInstance;
class BaseProfilerFactory;
class LaunchController;
class LauncherWindows;
class MinecraftInstance;
class QWidget;

/**
 * Operations LaunchCoordinator needs from the process that owns windows and controllers.
 * Application implements this. The coordinator does not include UI headers.
 */
class LaunchHost {
   public:
    virtual ~LaunchHost() = default;

    virtual bool saveInstanceWindow(const QString& instanceId) = 0;
    virtual QWidget* instanceWindow(const QString& instanceId) const = 0;
    virtual QWidget* mainWindow() const = 0;
    virtual void adoptController(const QString& instanceId, std::unique_ptr<LaunchController> controller) = 0;
    virtual LaunchController* controller(const QString& instanceId) const = 0;
    virtual BaseProfilerFactory* profiler(const QString& id) const = 0;
    virtual bool isUpdateRunning() const = 0;
    virtual void addRunningInstance() = 0;
    virtual void watch(LaunchController* controller) = 0;
    virtual LauncherWindows* windows() const = 0;
    virtual const QMap<QString, std::shared_ptr<BaseProfilerFactory>>& profilers() const = 0;
};

/** Starts and stops instance launches. Application owns one of these. */
class LaunchCoordinator : public QObject {
    Q_OBJECT
   public:
    explicit LaunchCoordinator(LaunchHost* host, QObject* parent = nullptr);

    bool launch(MinecraftInstance* instance,
                LaunchMode mode = LaunchMode::Normal,
                MinecraftTarget::Ptr targetToJoin = nullptr,
                MinecraftAccountPtr accountToUse = nullptr,
                const QString& offlineName = QString());
    bool kill(BaseInstance* instance);

    const QMap<QString, std::shared_ptr<BaseProfilerFactory>>& profilers() const;

   private:
    LaunchHost* m_host;
};
