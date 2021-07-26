/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "AccountTask.h"
#include "MinecraftAccount.h"

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QByteArray>

#include <Env.h>

#include <BuildConfig.h>

#include <QDebug>

AccountTask::AccountTask(AccountData *data, QObject *parent)
    : Task(parent), m_data(data)
{
    changeState(STATE_CREATED);
}

QString AccountTask::getStateMessage() const
{
    switch (m_accountState)
    {
    case STATE_CREATED:
        return "Waiting...";
    case STATE_WORKING:
        return tr("Sending request to auth servers...");
    case STATE_SUCCEEDED:
        return tr("Authentication task succeeded.");
    case STATE_FAILED_SOFT:
        return tr("Failed to contact the authentication server.");
    case STATE_FAILED_HARD:
        return tr("Failed to authenticate.");
    default:
        return tr("...");
    }
}

void AccountTask::changeState(AccountTask::State newState, QString reason)
{
    m_accountState = newState;
    setStatus(getStateMessage());
    if (newState == STATE_SUCCEEDED)
    {
        emitSucceeded();
    }
    else if (newState == STATE_FAILED_HARD || newState == STATE_FAILED_SOFT)
    {
        emitFailed(reason);
    }
}
