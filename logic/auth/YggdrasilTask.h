/* Copyright 2013 MultiMC Contributors
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

#include <logic/tasks/Task.h>

#include <QString>
#include <QJsonObject>

#include "logic/auth/MojangAccount.h"

class QNetworkReply;

/**
 * A Yggdrasil task is a task that performs an operation on a given mojang account.
 */
class YggdrasilTask : public Task
{
	Q_OBJECT
public:
	explicit YggdrasilTask(MojangAccountPtr account, QObject *parent = 0);
	~YggdrasilTask();

	/**
	 * Class describing a Yggdrasil error response.
	 */
	class Error
	{
	public:
		Error(const QString& shortError, const QString& errorMessage, const QString& cause) : 
			m_shortError(shortError), m_errorMessage(errorMessage), m_cause(cause) {}

		QString getShortError() const { return m_shortError; }
		QString getErrorMessage() const { return m_errorMessage; }
		QString getCause() const { return m_cause; }

		/// Gets the string to display in the GUI for describing this error.
		QString getDisplayMessage()
		{
			return getErrorMessage();
		}

	protected:
		QString m_shortError;
		QString m_errorMessage;
		QString m_cause;
	};

	/**
	 * Gets the Mojang account that this task is operating on.
	 */
	virtual MojangAccountPtr getMojangAccount() const;

	/**
	 * Returns a pointer to a YggdrasilTask::Error object if an error has occurred.
	 * If no error has occurred, returns a null pointer.
	 */
	virtual Error *getError() const;

protected:
	/**
	 * Enum for describing the state of the current task.
	 * Used by the getStateMessage function to determine what the status message should be.
	 */
	enum State
	{
		STATE_SENDING_REQUEST,
		STATE_PROCESSING_RESPONSE,
		STATE_OTHER,
	};

	virtual void executeTask();

	/**
	 * Gets the JSON object that will be sent to the authentication server.
	 * Should be overridden by subclasses.
	 */
	virtual QJsonObject getRequestContent() const = 0;

	/**
	 * Gets the endpoint to POST to.
	 * No leading slash.
	 */
	virtual QString getEndpoint() const = 0;

	/**
	 * Processes the response received from the server.
	 * If an error occurred, this should emit a failed signal and return false.
	 * If Yggdrasil gave an error response, it should call setError() first, and then return false.
	 * Otherwise, it should return true.
	 * Note: If the response from the server was blank, and the HTTP code was 200, this function is called with
	 * an empty QJsonObject.
	 */
	virtual bool processResponse(QJsonObject responseData) = 0;

	/**
	 * Processes an error response received from the server.
	 * The default implementation will read data from Yggdrasil's standard error response format and set it as this task's Error.
	 * \returns a QString error message that will be passed to emitFailed.
	 */
	virtual QString processError(QJsonObject responseData);

	/**
	 * Returns the state message for the given state.
	 * Used to set the status message for the task.
	 * Should be overridden by subclasses that want to change messages for a given state.
	 */
	virtual QString getStateMessage(const State state) const;

	MojangAccountPtr m_account;

	QNetworkReply *m_netReply;

	Error *m_error;

protected
slots:
	void processReply(QNetworkReply *reply);
};
