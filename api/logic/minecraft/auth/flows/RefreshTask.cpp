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

#include "RefreshTask.h"
#include "../MojangAccount.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>

#include <QDebug>

RefreshTask::RefreshTask(MojangAccount *account) : YggdrasilTask(account)
{
}

QJsonObject RefreshTask::getRequestContent() const
{
    /*
     * {
     *  "clientToken": "client identifier"
     *  "accessToken": "current access token to be refreshed"
     *  "selectedProfile":                      // specifying this causes errors
     *  {
     *   "id": "profile ID"
     *   "name": "profile name"
     *  }
     *  "requestUser": true/false               // request the user structure
     * }
     */
    QJsonObject req;
    req.insert("clientToken", m_account->m_clientToken);
    req.insert("accessToken", m_account->m_accessToken);
    /*
    {
        auto currentProfile = m_account->currentProfile();
        QJsonObject profile;
        profile.insert("id", currentProfile->id());
        profile.insert("name", currentProfile->name());
        req.insert("selectedProfile", profile);
    }
    */
    req.insert("requestUser", true);

    return req;
}

void RefreshTask::processResponse(QJsonObject responseData)
{
    // Read the response data. We need to get the client token, access token, and the selected
    // profile.
    qDebug() << "Processing authentication response.";

    // qDebug() << responseData;
    // If we already have a client token, make sure the one the server gave us matches our
    // existing one.
    QString clientToken = responseData.value("clientToken").toString("");
    if (clientToken.isEmpty())
    {
        // Fail if the server gave us an empty client token
        changeState(STATE_FAILED_HARD, tr("Authentication server didn't send a client token."));
        return;
    }
    if (!m_account->m_clientToken.isEmpty() && clientToken != m_account->m_clientToken)
    {
        changeState(STATE_FAILED_HARD, tr("Authentication server attempted to change the client token. This isn't supported."));
        return;
    }

    // Now, we set the access token.
    qDebug() << "Getting new access token.";
    QString accessToken = responseData.value("accessToken").toString("");
    if (accessToken.isEmpty())
    {
        // Fail if the server didn't give us an access token.
        changeState(STATE_FAILED_HARD, tr("Authentication server didn't send an access token."));
        return;
    }

    // we validate that the server responded right. (our current profile = returned current
    // profile)
    QJsonObject currentProfile = responseData.value("selectedProfile").toObject();
    QString currentProfileId = currentProfile.value("id").toString("");
    if (m_account->currentProfile()->id != currentProfileId)
    {
        changeState(STATE_FAILED_HARD, tr("Authentication server didn't specify the same prefile as expected."));
        return;
    }

    // this is what the vanilla launcher passes to the userProperties launch param
    if (responseData.contains("user"))
    {
        User u;
        auto obj = responseData.value("user").toObject();
        u.id = obj.value("id").toString();
        auto propArray = obj.value("properties").toArray();
        for (auto prop : propArray)
        {
            auto propTuple = prop.toObject();
            auto name = propTuple.value("name").toString();
            auto value = propTuple.value("value").toString();
            u.properties.insert(name, value);
        }
        m_account->m_user = u;
    }

    // We've made it through the minefield of possible errors. Return true to indicate that
    // we've succeeded.
    qDebug() << "Finished reading refresh response.";
    // Reset the access token.
    m_account->m_accessToken = accessToken;
    changeState(STATE_SUCCEEDED);
}

QString RefreshTask::getEndpoint() const
{
    return "refresh";
}

QString RefreshTask::getStateMessage() const
{
    switch (m_state)
    {
    case STATE_SENDING_REQUEST:
        return tr("Refreshing login token...");
    case STATE_PROCESSING_RESPONSE:
        return tr("Refreshing login token: Processing response...");
    default:
        return YggdrasilTask::getStateMessage();
    }
}
