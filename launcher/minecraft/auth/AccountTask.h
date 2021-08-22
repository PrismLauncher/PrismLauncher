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

#pragma once

#include <tasks/Task.h>

#include <QString>
#include <QJsonObject>
#include <QTimer>
#include <qsslerror.h>

#include "MinecraftAccount.h"

class QNetworkReply;

class AccountTask : public Task
{
    friend class AuthContext;
    Q_OBJECT
public:
    explicit AccountTask(AccountData * data, QObject *parent = 0);
    virtual ~AccountTask() {};

    /**
     * assign a session to this task. the session will be filled with required infomration
     * upon completion
     */
    void assignSession(AuthSessionPtr session)
    {
        m_session = session;
    }

    /// get the assigned session for filling with information.
    AuthSessionPtr getAssignedSession()
    {
        return m_session;
    }

    /**
     * Class describing a Account error response.
     */
    struct Error
    {
        QString m_errorMessageShort;
        QString m_errorMessageVerbose;
        QString m_cause;
    };

    enum AbortedBy
    {
        BY_NOTHING,
        BY_USER,
        BY_TIMEOUT
    } m_aborted = BY_NOTHING;

    /**
     * Enum for describing the state of the current task.
     * Used by the getStateMessage function to determine what the status message should be.
     */
    enum State
    {
        STATE_CREATED,
        STATE_WORKING,
        STATE_FAILED_SOFT, //!< soft failure. this generally means the user auth details haven't been invalidated
        STATE_FAILED_HARD, //!< hard failure. auth is invalid
        STATE_SUCCEEDED
    } m_accountState = STATE_CREATED;

    State accountState() {
        return m_accountState;
    }

signals:
    void showVerificationUriAndCode(const QUrl &uri, const QString &code, int expiresIn);
    void hideVerificationUriAndCode();

protected:

    /**
     * Returns the state message for the given state.
     * Used to set the status message for the task.
     * Should be overridden by subclasses that want to change messages for a given state.
     */
    virtual QString getStateMessage() const;

protected slots:
    void changeState(State newState, QString reason=QString());

protected:
    // FIXME: segfault disaster waiting to happen
    AccountData *m_data = nullptr;
    std::shared_ptr<Error> m_error;
    AuthSessionPtr m_session;
};
