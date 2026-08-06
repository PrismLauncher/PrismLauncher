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
 *      Authors: Orochimarufan <orochimarufan.x3@gmail.com>
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

#include "MinecraftAccount.h"

#include <QColor>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringList>
#include <QUuid>

#include <QDebug>

#include <QPainter>

#include "minecraft/auth/AccountData.h"
#include "minecraft/auth/AuthFlow.h"

MinecraftAccount::MinecraftAccount(QObject* parent) : QObject(parent)
{
    data.internalId = QUuid::createUuid().toString(QUuid::Id128);
}

MinecraftAccountPtr MinecraftAccount::loadFromJsonV3(const QJsonObject& json)
{
    MinecraftAccountPtr account(new MinecraftAccount());
    if (account->data.resumeStateFromV3(json)) {
        return account;
    }
    return nullptr;
}

MinecraftAccountPtr MinecraftAccount::createBlankMSA()
{
    return createBlank(AccountType::MSA);
}

MinecraftAccountPtr MinecraftAccount::createBlank(AccountType type)
{
    Q_ASSERT(type == AccountType::MSA || type == AccountType::Ely);
    MinecraftAccountPtr account(new MinecraftAccount());
    account->data.type = type;
    return account;
}

QJsonObject MinecraftAccount::saveToJson() const
{
    return data.saveState();
}

AccountState MinecraftAccount::accountState() const
{
    return data.accountState;
}

QPixmap MinecraftAccount::getFace(int width, int height) const
{
    QPixmap skinTexture;
    if (!skinTexture.loadFromData(data.minecraftProfile.skin.data, "PNG")) {
        return QPixmap();
    }
    QPixmap skin = QPixmap(8, 8);
    skin.fill(QColorConstants::Transparent);
    QPainter painter(&skin);
    painter.drawPixmap(0, 0, skinTexture.copy(8, 8, 8, 8));
    painter.drawPixmap(0, 0, skinTexture.copy(40, 8, 8, 8));
    return skin.scaled(width, height, Qt::KeepAspectRatio);
}

shared_qobject_ptr<AuthFlow> MinecraftAccount::login(bool useDeviceCode)
{
    Q_ASSERT(m_currentTask.get() == nullptr);

    m_currentTask.reset(new AuthFlow(&data, useDeviceCode ? AuthFlow::Action::DeviceCode : AuthFlow::Action::Login));
    connect(m_currentTask.get(), &Task::succeeded, this, &MinecraftAccount::authSucceeded);
    connect(m_currentTask.get(), &Task::failed, this, &MinecraftAccount::authFailed);
    connect(m_currentTask.get(), &Task::aborted, this, [this] { authFailed(tr("Aborted")); });
    emit activityChanged(true);
    return m_currentTask;
}

shared_qobject_ptr<AuthFlow> MinecraftAccount::refresh()
{
    if (m_currentTask) {
        return m_currentTask;
    }

    m_currentTask.reset(new AuthFlow(&data, AuthFlow::Action::Refresh));

    connect(m_currentTask.get(), &Task::succeeded, this, &MinecraftAccount::authSucceeded);
    connect(m_currentTask.get(), &Task::failed, this, &MinecraftAccount::authFailed);
    connect(m_currentTask.get(), &Task::aborted, this, [this] { authFailed(tr("Aborted")); });
    emit activityChanged(true);
    return m_currentTask;
}

shared_qobject_ptr<AuthFlow> MinecraftAccount::currentTask()
{
    return m_currentTask;
}

void MinecraftAccount::authSucceeded()
{
    m_currentTask.reset();
    emit changed();
    emit activityChanged(false);
}

void MinecraftAccount::authFailed(QString reason)
{
    switch (m_currentTask->taskState()) {
        case AccountTaskState::STATE_OFFLINE:
        case AccountTaskState::STATE_DISABLED: {
            // NOTE: user will need to fix this themselves.
        }
        case AccountTaskState::STATE_FAILED_SOFT: {
            // NOTE: this doesn't do much. There was an error of some sort.
        } break;
        case AccountTaskState::STATE_FAILED_HARD: {
            data.msaToken.token = QString();
            data.msaToken.refresh_token = QString();
            data.msaToken.validity = Validity::None;
            data.yggdrasilToken.token = QString();
            data.yggdrasilToken.validity = Validity::None;
            data.validity_ = Validity::None;
            emit changed();
        } break;
        case AccountTaskState::STATE_FAILED_GONE: {
            data.validity_ = Validity::None;
            emit changed();
        } break;
        case AccountTaskState::STATE_WORKING: {
            data.accountState = AccountState::Unchecked;
        } break;
        case AccountTaskState::STATE_CREATED:
        case AccountTaskState::STATE_SUCCEEDED: {
            // Not reachable here, as they are not failures.
        }
    }
    m_currentTask.reset();
    emit activityChanged(false);
}

QString MinecraftAccount::displayName() const
{
    if (const QList validStates{ AccountState::Unchecked, AccountState::Working, AccountState::Offline, AccountState::Online }; !validStates.contains(accountState())) {
        return QString("⚠ %1").arg(profileName());
    }
    return profileName();
}

bool MinecraftAccount::isActive() const
{
    return !m_currentTask.isNull();
}

bool MinecraftAccount::shouldRefresh() const
{
    /*
     * Never refresh accounts that are being used by the game, it breaks the game session.
     * Always refresh accounts that have not been refreshed yet during this session.
     * Don't refresh broken accounts.
     * Refresh accounts that would expire in the next 12 hours (fresh token validity is 24 hours).
     */
    if (isInUse()) {
        return false;
    }
    switch (data.validity_) {
        case Validity::Certain: {
            break;
        }
        case Validity::None: {
            return false;
        }
        case Validity::Assumed: {
            return true;
        }
    }
    auto now = QDateTime::currentDateTimeUtc();
    auto issuedTimestamp = data.yggdrasilToken.issueInstant;
    auto expiresTimestamp = data.yggdrasilToken.notAfter;

    if (!expiresTimestamp.isValid()) {
        expiresTimestamp = issuedTimestamp.addSecs(24 * 3600);
    }
    if (now.secsTo(expiresTimestamp) < (12 * 3600)) {
        return true;
    }
    return false;
}

void MinecraftAccount::fillSession(AuthSessionPtr session)
{
    // volatile auth token
    session->access_token = data.accessToken();
    // profile name
    session->player_name = data.profileName();
    // profile ID
    session->uuid = data.profileId();
    // Minecraft-compatible user type and optional Ely.by runtime patch.
    session->user_type = typeString();
    session->wantsElyPatch = data.type == AccountType::Ely;
    if (!session->access_token.isEmpty()) {
        session->session = "token:" + data.accessToken() + ":" + data.profileId();
    } else {
        session->session = "-";
    }
}

void MinecraftAccount::decrementUses()
{
    Usable::decrementUses();
    if (!isInUse()) {
        emit changed();
        // FIXME: we now need a better way to identify accounts...
        qWarning() << "Profile" << data.profileId() << "is no longer in use.";
    }
}

void MinecraftAccount::incrementUses()
{
    bool wasInUse = isInUse();
    Usable::incrementUses();
    if (!wasInUse) {
        emit changed();
        // FIXME: we now need a better way to identify accounts...
        qWarning() << "Profile" << data.profileId() << "is now in use.";
    }
}

QUuid MinecraftAccount::uuidFromUsername(QString username)
{
    const auto input = QString("OfflinePlayer:%1").arg(username).toUtf8();
    QByteArray digest = QCryptographicHash::hash(input, QCryptographicHash::Md5);

    auto setBits = [](QByteArray& bytes, qsizetype index, uint8_t clearMask, uint8_t setMask) {
        bytes[index] &= clearMask;
        bytes[index] |= setMask;
    };
    setBits(digest, 6, 0x0f, 0x30);  // version 3
    setBits(digest, 8, 0x3f, 0x80);  // IETF variant
    return QUuid::fromRfc4122(digest);
}
