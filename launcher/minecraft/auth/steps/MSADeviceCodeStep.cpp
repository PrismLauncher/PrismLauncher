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
#include "Json.h"
#include "net/RawHeaderProxy.h"

// https://learn.microsoft.com/en-us/entra/identity-platform/v2-oauth2-device-code
MSADeviceCodeStep::MSADeviceCodeStep(AccountData* data) : AuthStep(data)
{
    m_clientId = APPLICATION->getMSAClientID();
    connect(&m_expiration_timer, &QTimer::timeout, this, &MSADeviceCodeStep::abort);
    connect(&m_pool_timer, &QTimer::timeout, this, &MSADeviceCodeStep::authenticateUser);
}

QString MSADeviceCodeStep::describe()
{
    return tr("Logging in with Microsoft account(device code).");
}

void MSADeviceCodeStep::perform()
{
    QUrlQuery data;
    data.addQueryItem("client_id", m_clientId);
    data.addQueryItem("scope", "XboxLive.SignIn XboxLive.offline_access");
    auto payload = data.query(QUrl::FullyEncoded).toUtf8();
    QUrl url("https://login.microsoftonline.com/consumers/oauth2/v2.0/devicecode");
    auto headers = QList<Net::HeaderPair>{
        { "Content-Type", "application/x-www-form-urlencoded" },
        { "Accept", "application/json" },
    };
    m_response.reset(new QByteArray());
    m_request = Net::Upload::makeByteArray(url, m_response, payload);
    m_request->addHeaderProxy(new Net::RawHeaderProxy(headers));

    m_task.reset(new NetJob("MSADeviceCodeStep", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);

    connect(m_task.get(), &Task::finished, this, &MSADeviceCodeStep::deviceAutorizationFinished);

    m_task->start();
}

struct DeviceAutorizationResponse {
    QString device_code;
    QString user_code;
    QString verification_uri;
    int expires_in;
    int interval;

    QString error;
    QString error_description;
};

DeviceAutorizationResponse parseDeviceAutorizationResponse(const QByteArray& data)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse device autorization response due to err:" << err.errorString();
        return {};
    }

    if (!doc.isObject()) {
        qWarning() << "Device autorization response is not an object";
        return {};
    }
    auto obj = doc.object();
    return {
        Json::ensureString(obj, "device_code"),       Json::ensureString(obj, "user_code"), Json::ensureString(obj, "verification_uri"),
        Json::ensureInteger(obj, "expires_in"),       Json::ensureInteger(obj, "interval"), Json::ensureString(obj, "error"),
        Json::ensureString(obj, "error_description"),
    };
}

void MSADeviceCodeStep::deviceAutorizationFinished()
{
    auto rsp = parseDeviceAutorizationResponse(*m_response);
    if (!rsp.error.isEmpty() || !rsp.error_description.isEmpty()) {
        qWarning() << "Device authorization failed:" << rsp.error;
        emit finished(AccountTaskState::STATE_FAILED_HARD,
                      tr("Device authorization failed: %1").arg(rsp.error_description.isEmpty() ? rsp.error : rsp.error_description));
        return;
    }
    if (!m_request->wasSuccessful() || m_request->error() != QNetworkReply::NoError) {
        emit finished(AccountTaskState::STATE_FAILED_HARD, tr("Failed to retrieve device authorization"));
        qDebug() << *m_response;
        return;
    }

    if (rsp.device_code.isEmpty() || rsp.user_code.isEmpty() || rsp.verification_uri.isEmpty() || rsp.expires_in == 0) {
        emit finished(AccountTaskState::STATE_FAILED_HARD, tr("Device authorization failed: required fields missing"));
        return;
    }
    if (rsp.interval != 0) {
        interval = rsp.interval;
    }
    m_device_code = rsp.device_code;
    emit authorizeWithBrowser(rsp.verification_uri, rsp.user_code, rsp.expires_in);
    m_expiration_timer.setTimerType(Qt::VeryCoarseTimer);
    m_expiration_timer.setInterval(rsp.expires_in * 1000);
    m_expiration_timer.setSingleShot(true);
    m_expiration_timer.start();
    m_pool_timer.setTimerType(Qt::VeryCoarseTimer);
    m_pool_timer.setSingleShot(true);
    startPoolTimer();
}

void MSADeviceCodeStep::abort()
{
    m_expiration_timer.stop();
    m_pool_timer.stop();
    if (m_request) {
        m_request->abort();
    }
    m_is_aborted = true;
    emit finished(AccountTaskState::STATE_FAILED_HARD, tr("Task aborted"));
}

void MSADeviceCodeStep::startPoolTimer()
{
    if (m_is_aborted) {
        return;
    }
    if (m_expiration_timer.remainingTime() < interval * 1000) {
        perform();
        return;
    }

    m_pool_timer.setInterval(interval * 1000);
    m_pool_timer.start();
}

void MSADeviceCodeStep::authenticateUser()
{
    QUrlQuery data;
    data.addQueryItem("client_id", m_clientId);
    data.addQueryItem("grant_type", "urn:ietf:params:oauth:grant-type:device_code");
    data.addQueryItem("device_code", m_device_code);
    auto payload = data.query(QUrl::FullyEncoded).toUtf8();
    QUrl url("https://login.microsoftonline.com/consumers/oauth2/v2.0/token");
    auto headers = QList<Net::HeaderPair>{
        { "Content-Type", "application/x-www-form-urlencoded" },
        { "Accept", "application/json" },
    };
    m_response.reset(new QByteArray());
    m_request = Net::Upload::makeByteArray(url, m_response, payload);
    m_request->addHeaderProxy(new Net::RawHeaderProxy(headers));

    connect(m_request.get(), &Task::finished, this, &MSADeviceCodeStep::authenticationFinished);

    m_request->setNetwork(APPLICATION->network());
    m_request->start();
}

struct AuthenticationResponse {
    QString access_token;
    QString token_type;
    QString refresh_token;
    int expires_in;

    QString error;
    QString error_description;

    QVariantMap extra;
};

AuthenticationResponse parseAuthenticationResponse(const QByteArray& data)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse device autorization response due to err:" << err.errorString();
        return {};
    }

    if (!doc.isObject()) {
        qWarning() << "Device autorization response is not an object";
        return {};
    }
    auto obj = doc.object();
    return { Json::ensureString(obj, "access_token"),
             Json::ensureString(obj, "token_type"),
             Json::ensureString(obj, "refresh_token"),
             Json::ensureInteger(obj, "expires_in"),
             Json::ensureString(obj, "error"),
             Json::ensureString(obj, "error_description"),
             obj.toVariantMap() };
}

void MSADeviceCodeStep::authenticationFinished()
{
    if (m_request->error() == QNetworkReply::TimeoutError) {
        // rfc8628#section-3.5
        // "On encountering a connection timeout, clients MUST unilaterally
        // reduce their polling frequency before retrying.  The use of an
        // exponential backoff algorithm to achieve this, such as doubling the
        // polling interval on each such connection timeout, is RECOMMENDED."
        interval *= 2;
        startPoolTimer();
        return;
    }
    auto rsp = parseAuthenticationResponse(*m_response);
    if (rsp.error == "slow_down") {
        // rfc8628#section-3.5
        // "A variant of 'authorization_pending', the authorization request is
        // still pending and polling should continue, but the interval MUST
        // be increased by 5 seconds for this and all subsequent requests."
        interval += 5;
        startPoolTimer();
        return;
    }
    if (rsp.error == "authorization_pending") {
        // keep trying - rfc8628#section-3.5
        // "The authorization request is still pending as the end user hasn't
        // yet completed the user-interaction steps (Section 3.3)."
        startPoolTimer();
        return;
    }
    if (!rsp.error.isEmpty() || !rsp.error_description.isEmpty()) {
        qWarning() << "Device Access failed:" << rsp.error;
        emit finished(AccountTaskState::STATE_FAILED_HARD,
                      tr("Device Access failed: %1").arg(rsp.error_description.isEmpty() ? rsp.error : rsp.error_description));
        return;
    }
    if (!m_request->wasSuccessful() || m_request->error() != QNetworkReply::NoError) {
        startPoolTimer();  // it failed so just try again without increasing the interval
        return;
    }

    m_expiration_timer.stop();
    m_data->msaClientID = m_clientId;
    m_data->msaToken.issueInstant = QDateTime::currentDateTimeUtc();
    m_data->msaToken.notAfter = QDateTime::currentDateTime().addSecs(rsp.expires_in);
    m_data->msaToken.extra = rsp.extra;
    m_data->msaToken.refresh_token = rsp.refresh_token;
    m_data->msaToken.token = rsp.access_token;
    emit finished(AccountTaskState::STATE_WORKING, tr("Got"));
}