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

#include "Gamepad.h"

#include <QCursor>
#include <QEvent>
#include <QFile>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QtCore/QDebug>
#include <QtGui/QWindow>

#include "Application.h"

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_gamecontroller.h>
#include <SDL_stdinc.h>
// Reset bool redefinition from SDL header
#undef bool

FILE* QFileToFilePointer(const QString& filePath, const char* mode)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        // Error handling: failed to open the file
        return nullptr;
    }

    // Get the file descriptor of the QFile
    qint64 fileDescriptor = file.handle();
    if (fileDescriptor == -1) {
        // Error handling: failed to get the file descriptor
        return nullptr;
    }

    // Convert the file descriptor to a FILE pointer
    FILE* filePtr = fdopen(fileDescriptor, mode);
    if (!filePtr) {
        // Error handling: failed to convert file descriptor to FILE pointer
        return nullptr;
    }

    return filePtr;
}

GamepadManager::GamepadManager(QObject* parent) : QObject(parent)
{
    // Initialize SDL with necessary subsystems for gamepad support
    if (SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK)) {
        qDebug() << SDL_GetError();
        return;
    }
    connect(&m_event_loop_timer, &QTimer::timeout, this, &GamepadManager::loop);
    m_event_loop_timer.start(16);

    FILE* filePtr = QFileToFilePointer(":/documents/libraries/SDL_GameControllerDB/gamecontrollerdb.txt", "r");
    if (filePtr) {
        // Error handling: failed to convert QFile to FILE pointer
        if (SDL_GameControllerAddMappingsFromRW(SDL_RWFromFP(filePtr, SDL_TRUE), 1) < 0) {
            qWarning("Could not load controller mappings: %s", SDL_GetError());
        }
    }
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            m_controller = SDL_GameControllerOpen(i);
            auto joystick = SDL_GameControllerGetJoystick(m_controller);
            m_instance_id = SDL_JoystickInstanceID(joystick);
            break;
        }
    }
}
void GamepadManager::loop()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_CONTROLLERAXISMOTION) {
            int axis = event.caxis.axis;
            int value = event.caxis.value;

            if (axis == SDL_CONTROLLER_AXIS_LEFTX || axis == SDL_CONTROLLER_AXIS_LEFTY) {
                // Map axis values to mouse movement
                int deltaX = (axis == SDL_CONTROLLER_AXIS_LEFTX) ? value / 3276.7 : 0;
                int deltaY = (axis == SDL_CONTROLLER_AXIS_LEFTY) ? value / 3276.7 : 0;

                QPoint globalPos = QCursor::pos();
                QCursor::setPos(globalPos.x() + deltaX, globalPos.y() + deltaY);
            }
        } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
            auto buttonEvent = event.cbutton;
            buttonPress(buttonEvent.button);
        } else if (event.type == SDL_CONTROLLERDEVICEADDED) {
            if (!m_controller) {
                for (int i = 0; i < SDL_NumJoysticks(); i++) {
                    if (SDL_IsGameController(i)) {
                        m_controller = SDL_GameControllerOpen(i);
                        auto joystick = SDL_GameControllerGetJoystick(m_controller);
                        m_instance_id = SDL_JoystickInstanceID(joystick);
                        break;
                    }
                }
            }
        } else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
            auto deviceEvent = event.cdevice;
            if (deviceEvent.which == m_instance_id) {
                SDL_GameControllerClose(m_controller);
                m_instance_id = 0;
            }
            for (int i = 0; i < SDL_NumJoysticks(); i++) {
                if (SDL_IsGameController(i)) {
                    m_controller = SDL_GameControllerOpen(i);
                    auto joystick = SDL_GameControllerGetJoystick(m_controller);
                    m_instance_id = SDL_JoystickInstanceID(joystick);
                    break;
                }
            }
        }
    }
}
GamepadManager::~GamepadManager()
{
    m_event_loop_timer.stop();
    if (m_controller)
        SDL_GameControllerClose(m_controller);
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK);
    SDL_Quit();
}
void GamepadManager::buttonPress(int button)
{
    switch (button) {
        case SDL_CONTROLLER_BUTTON_A:
            sendKeyButtonEvent(Qt::Key_Return);
            break;
        case SDL_CONTROLLER_BUTTON_B:
            sendKeyButtonEvent(Qt::Key_Back);
            break;
        case SDL_CONTROLLER_BUTTON_X:
            sendMouseButtonEvent(Qt::LeftButton);
            break;
        case SDL_CONTROLLER_BUTTON_Y:
            // sendMouseButtonEvent(Qt::RightButton); //broken at the moment
            break;
        case SDL_CONTROLLER_BUTTON_BACK:
            sendKeyButtonEvent(Qt::Key_Back);
            break;
        case SDL_CONTROLLER_BUTTON_GUIDE:
            break;
        case SDL_CONTROLLER_BUTTON_START:
            break;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK:
            break;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
            break;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            break;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            sendKeyButtonEvent(Qt::Key_Up);
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            sendKeyButtonEvent(Qt::Key_Down);
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            sendKeyButtonEvent(Qt::Key_Left);
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            sendKeyButtonEvent(Qt::Key_Right);
            break;
        default:
            break;
    }
}

void GamepadManager::sendEvent(QEvent* event)
{
    auto focusWindow = APPLICATION->focusWindow();
    if (focusWindow) {
        QGuiApplication::sendEvent(focusWindow, event);
    } else {
        delete event;
    }
}
void GamepadManager::sendMouseButtonEvent(Qt::MouseButton button)
{
    auto focusWindow = APPLICATION->focusWindow();
    if (focusWindow) {
        auto pos = focusWindow->mapFromGlobal(QCursor::pos());
        sendEvent(new QMouseEvent(QEvent::MouseButtonPress, pos, button, button, Qt::NoModifier));
        sendEvent(new QMouseEvent(QEvent::MouseButtonRelease, pos, button, button, Qt::NoModifier));
    }
}
void GamepadManager::sendKeyButtonEvent(Qt::Key key)
{
    sendEvent(new QKeyEvent(QEvent::KeyPress, key, Qt::NoModifier));
}
