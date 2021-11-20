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

#include "../AccountTask.h"

#include <QString>
#include <QJsonObject>
#include <QTimer>
#include <qsslerror.h>

#include "../MinecraftAccount.h"

class QNetworkAccessManager;
class QNetworkReply;

/**
 * A Yggdrasil task is a task that performs an operation on a given mojang account.
 */
class Yggdrasil : public AccountTask
{
    Q_OBJECT
public:
    explicit Yggdrasil(
        AccountData *data,
        QObject *parent = 0
    );
    virtual ~Yggdrasil() {};

    void refresh();
    void login(QString password);
protected:
    void executeTask() override;

    /**
     * Processes the response received from the server.
     * If an error occurred, this should emit a failed signal.
     * If Yggdrasil gave an error response, it should call setError() first, and then return false.
     * Otherwise, it should return true.
     * Note: If the response from the server was blank, and the HTTP code was 200, this function is called with
     * an empty QJsonObject.
     */
    void processResponse(QJsonObject responseData);

    /**
     * Processes an error response received from the server.
     * The default implementation will read data from Yggdrasil's standard error response format and set it as this task's Error.
     * \returns a QString error message that will be passed to emitFailed.
     */
    virtual void processError(QJsonObject responseData);

protected slots:
    void processReply();
    void refreshTimers(qint64, qint64);
    void heartbeat();
    void sslErrors(QList<QSslError>);
    void abortByTimeout();

public slots:
    virtual bool abort() override;

private:
    void sendRequest(QUrl endpoint, QByteArray content);

protected:
    QNetworkReply *m_netReply = nullptr;
    QTimer timeout_keeper;
    QTimer counter;
    int count = 0; // num msec since time reset

    const int timeout_max = 30000;
    const int time_step = 50;
};
