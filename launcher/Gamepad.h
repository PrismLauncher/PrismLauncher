#pragma once
// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 Trial97 <alexandru.tripon97@gmail.com>
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

#include <QObject>
#include <QTimer>

#include <SDL_gamecontroller.h>

class GamepadManager : public QObject {
    Q_OBJECT
   public:
    GamepadManager(QObject* parent = nullptr);
    virtual ~GamepadManager();

   private slots:
    void loop();
    void buttonPress(int button);

    void sendEvent(QEvent* event);
    void sendMouseButtonEvent(Qt::MouseButton button);
    void sendKeyButtonEvent(Qt::Key key);

   private:
    QTimer m_event_loop_timer;
    SDL_GameController* m_controller;
    int m_instance_id;
};