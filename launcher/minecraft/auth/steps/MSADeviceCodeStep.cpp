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

#include "MSADeviceCodeStep.h"

#include <QDateTime>
#include <QUrlQuery>

#include "Application.h"

MSADeviceCodeStep::MSADeviceCodeStep(AccountData* data) : AuthStep(data)
{
    m_clientId = APPLICATION->getMSAClientID();
    m_oauth2.setAuthorizationUrl(QUrl("https://login.microsoftonline.com/consumers/oauth2/v2.0/devicecode"));
    m_oauth2.setTokenUrl(QUrl("https://login.microsoftonline.com/consumers/oauth2/v2.0/token"));
    m_oauth2.setRequestedScopeTokens({ "XboxLive.SignIn", "XboxLive.offline_access" });
    m_oauth2.setClientIdentifier(m_clientId);
    m_oauth2.setNetworkAccessManager(APPLICATION->network());

    connect(&m_oauth2, &QOAuth2DeviceAuthorizationFlow::granted, this, [this] {
        m_data->msaClientID = m_oauth2.clientIdentifier();
        m_data->msaToken.issueInstant = QDateTime::currentDateTimeUtc();
        m_data->msaToken.notAfter = m_oauth2.expirationAt();
        m_data->msaToken.extra = m_oauth2.extraTokens();
        m_data->msaToken.refresh_token = m_oauth2.refreshToken();
        m_data->msaToken.token = m_oauth2.token();
        emit finished(AccountTaskState::STATE_WORKING, tr("Got "));
    });
    connect(&m_oauth2, &QOAuth2DeviceAuthorizationFlow::authorizeWithUserCode, this, &MSADeviceCodeStep::authorizeWithBrowser);
    connect(&m_oauth2, &QOAuth2DeviceAuthorizationFlow::requestFailed, this, [this](const QAbstractOAuth2::Error err) {
        auto state = AccountTaskState::STATE_FAILED_HARD;
        if (m_oauth2.status() == QAbstractOAuth::Status::Granted) {
            if (err == QAbstractOAuth2::Error::NetworkError) {
                state = AccountTaskState::STATE_OFFLINE;
            } else {
                state = AccountTaskState::STATE_FAILED_SOFT;
            }
        }
        auto message = tr("Microsoft user authentication failed.");
        qWarning() << message;
        emit finished(state, message);
    });
    connect(&m_oauth2, &QOAuth2DeviceAuthorizationFlow::serverReportedErrorOccurred, this,
            [this](const QString& error, const QString& errorDescription, const QUrl& /*uri*/) {
                qWarning() << "Failed to login because" << error << errorDescription;
                emit finished(AccountTaskState::STATE_FAILED_HARD, errorDescription);
            });

    connect(&m_oauth2, &QOAuth2DeviceAuthorizationFlow::extraTokensChanged, this,
            [this](const QVariantMap& tokens) { m_data->msaToken.extra = tokens; });

    connect(&m_oauth2, &QOAuth2DeviceAuthorizationFlow::clientIdentifierChanged, this,
            [this](const QString& clientIdentifier) { m_data->msaClientID = clientIdentifier; });
    connect(&m_oauth2, &QOAuth2DeviceAuthorizationFlow::userCodeExpirationAtChanged, this, [](const QDateTime& expiration) {
        qDebug() << "=============";
        qDebug() << expiration;
        qDebug() << "=============";
    });
}

QString MSADeviceCodeStep::describe()
{
    return tr("Logging in with Microsoft account(device code).");
}

void MSADeviceCodeStep::perform()
{
    *m_data = AccountData();
    m_data->msaClientID = m_clientId;
    m_oauth2.grant();
}

void MSADeviceCodeStep::abort()
{
    if (m_oauth2.isPolling()) {
        m_oauth2.stopTokenPolling();
    }

    emit finished(AccountTaskState::STATE_FAILED_HARD, tr("Task aborted"));
}
