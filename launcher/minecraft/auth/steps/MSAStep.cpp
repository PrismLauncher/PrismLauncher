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

#include "MSAStep.h"

#include <QtNetworkAuth/qoauthhttpserverreplyhandler.h>
#include <QAbstractOAuth2>
#include <QNetworkRequest>

#include "Application.h"

MSAStep::MSAStep(AccountData* data, bool silent) : AuthStep(data), m_silent(silent)
{
    m_clientId = APPLICATION->getMSAClientID();

    auto replyHandler = new QOAuthHttpServerReplyHandler(1337, this);
    replyHandler->setCallbackText(
        " <iframe src=\"https://prismlauncher.org/successful-login\" title=\"PrismLauncher Microsoft login\" style=\"position:fixed; "
        "top:0; left:0; bottom:0; right:0; width:100%; height:100%; border:none; margin:0; padding:0; overflow:hidden; "
        "z-index:999999;\"/> ");
    oauth2.setReplyHandler(replyHandler);
    oauth2.setAuthorizationUrl(QUrl("https://login.microsoftonline.com/consumers/oauth2/v2.0/authorize"));
    oauth2.setAccessTokenUrl(QUrl("https://login.microsoftonline.com/consumers/oauth2/v2.0/token"));
    oauth2.setScope("XboxLive.SignIn XboxLive.offline_access");
    oauth2.setClientIdentifier(m_clientId);
    oauth2.setNetworkAccessManager(APPLICATION->network().get());

    connect(&oauth2, &QOAuth2AuthorizationCodeFlow::granted, this, [this] {
        m_data->msaClientID = oauth2.clientIdentifier();
        m_data->msaToken.issueInstant = QDateTime::currentDateTimeUtc();
        m_data->msaToken.notAfter = oauth2.expirationAt();
        m_data->msaToken.extra = oauth2.extraTokens();
        m_data->msaToken.refresh_token = oauth2.refreshToken();
        m_data->msaToken.token = oauth2.token();
        emit finished(AccountTaskState::STATE_WORKING, tr("Got "));
    });
    connect(&oauth2, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, this, &MSAStep::authorizeWithBrowser);
    connect(&oauth2, &QOAuth2AuthorizationCodeFlow::requestFailed, this, [this](const QAbstractOAuth2::Error err) {
        emit finished(AccountTaskState::STATE_FAILED_HARD, tr("Microsoft user authentication failed."));
    });

    connect(&oauth2, &QOAuth2AuthorizationCodeFlow::extraTokensChanged, this,
            [this](const QVariantMap& tokens) { m_data->msaToken.extra = tokens; });

    connect(&oauth2, &QOAuth2AuthorizationCodeFlow::clientIdentifierChanged, this,
            [this](const QString& clientIdentifier) { m_data->msaClientID = clientIdentifier; });
}

QString MSAStep::describe()
{
    return tr("Logging in with Microsoft account.");
}

void MSAStep::perform()
{
    if (m_silent) {
        if (m_data->msaClientID != m_clientId) {
            emit finished(AccountTaskState::STATE_DISABLED,
                          tr("Microsoft user authentication failed - client identification has changed."));
        }
        oauth2.setRefreshToken(m_data->msaToken.refresh_token);
        oauth2.refreshAccessToken();
    } else {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)  // QMultiMap param changed in 6.0
        oauth2.setModifyParametersFunction([](QAbstractOAuth::Stage stage, QMultiMap<QString, QVariant>* map) {
#else
        oauth2.setModifyParametersFunction([](QAbstractOAuth::Stage stage, QMap<QString, QVariant>* map) {
#endif
            map->insert("prompt", "select_account");
        });

        *m_data = AccountData();
        m_data->msaClientID = m_clientId;
        oauth2.grant();
    }
}
