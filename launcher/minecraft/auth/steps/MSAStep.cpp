// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
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

#include <QNetworkRequest>

#include "BuildConfig.h"
#include "minecraft/auth/AuthRequest.h"
#include "minecraft/auth/Parsers.h"

#include "Application.h"

using OAuth2 = Katabasis::DeviceFlow;
using Activity = Katabasis::Activity;

MSAStep::MSAStep(AccountData* data, Action action) : AuthStep(data), m_action(action) {
    m_clientId = APPLICATION->getMSAClientID();
    OAuth2::Options opts;
    opts.scope = "XboxLive.signin offline_access";
    opts.clientIdentifier = m_clientId;
    opts.authorizationUrl = "https://login.microsoftonline.com/consumers/oauth2/v2.0/devicecode";
    opts.accessTokenUrl = "https://login.microsoftonline.com/consumers/oauth2/v2.0/token";

    // FIXME: OAuth2 is not aware of our fancy shared pointers
    m_oauth2 = new OAuth2(opts, m_data->msaToken, this, APPLICATION->network().get());

    connect(m_oauth2, &OAuth2::activityChanged, this, &MSAStep::onOAuthActivityChanged);
    connect(m_oauth2, &OAuth2::showVerificationUriAndCode, this, &MSAStep::showVerificationUriAndCode);
}

MSAStep::~MSAStep() noexcept = default;

QString MSAStep::describe() {
    return tr("Logging in with Microsoft account.");
}


void MSAStep::rehydrate() {
    switch(m_action) {
        case Refresh: {
            // TODO: check the tokens and see if they are old (older than a day)
            return;
        }
        case Login: {
            // NOOP
            return;
        }
    }
}

void MSAStep::perform() {
    switch(m_action) {
        case Refresh: {
            if (m_data->msaClientID != m_clientId) {
                emit hideVerificationUriAndCode();
                emit finished(AccountTaskState::STATE_DISABLED, tr("Microsoft user authentication failed - client identification has changed."));
            }
            m_oauth2->refresh();
            return;
        }
        case Login: {
            QVariantMap extraOpts;
            extraOpts["prompt"] = "select_account";
            m_oauth2->setExtraRequestParams(extraOpts);

            *m_data = AccountData();
            m_data->msaClientID = m_clientId;
            m_oauth2->login();
            return;
        }
    }
}

void MSAStep::onOAuthActivityChanged(Katabasis::Activity activity) {
    switch(activity) {
        case Katabasis::Activity::Idle:
        case Katabasis::Activity::LoggingIn:
        case Katabasis::Activity::Refreshing:
        case Katabasis::Activity::LoggingOut: {
            // We asked it to do something, it's doing it. Nothing to act upon.
            return;
        }
        case Katabasis::Activity::Succeeded: {
            // Succeeded or did not invalidate tokens
            emit hideVerificationUriAndCode();
            QVariantMap extraTokens = m_oauth2->extraTokens();
#ifndef NDEBUG
            if (!extraTokens.isEmpty()) {
                qDebug() << "Extra tokens in response:";
                foreach (QString key, extraTokens.keys()) {
                    qDebug() << "\t" << key << ":" << extraTokens.value(key);
                }
            }
#endif
            emit finished(AccountTaskState::STATE_WORKING, tr("Got "));
            return;
        }
        case Katabasis::Activity::FailedSoft: {
            // NOTE: soft error in the first step means 'offline'
            emit hideVerificationUriAndCode();
            emit finished(AccountTaskState::STATE_OFFLINE, tr("Microsoft user authentication ended with a network error."));
            return;
        }
        case Katabasis::Activity::FailedGone: {
            emit hideVerificationUriAndCode();
            emit finished(AccountTaskState::STATE_FAILED_GONE, tr("Microsoft user authentication failed - user no longer exists."));
            return;
        }
        case Katabasis::Activity::FailedHard: {
            emit hideVerificationUriAndCode();
            emit finished(AccountTaskState::STATE_FAILED_HARD, tr("Microsoft user authentication failed."));
            return;
        }
        default: {
            emit hideVerificationUriAndCode();
            emit finished(AccountTaskState::STATE_FAILED_HARD, tr("Microsoft user authentication completed with an unrecognized result."));
            return;
        }
    }
}
