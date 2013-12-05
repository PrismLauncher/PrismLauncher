
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

#include <logic/auth/flows/ValidateTask.h>

#include <logic/auth/MojangAccount.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QDebug>

#include "logger/QsLog.h"

ValidateTask::ValidateTask(MojangAccount * account, QObject *parent)
	: YggdrasilTask(account, parent)
{
}

QJsonObject ValidateTask::getRequestContent() const
{
	QJsonObject req;
	req.insert("accessToken", m_account->m_accessToken);
	return req;
}

bool ValidateTask::processResponse(QJsonObject responseData)
{
	// Assume that if processError wasn't called, then the request was successful.
	emitSucceeded();
	return true;
}

QString ValidateTask::getEndpoint() const
{
	return "validate";
}

QString ValidateTask::getStateMessage(const YggdrasilTask::State state) const
{
	switch (state)
	{
	case STATE_SENDING_REQUEST:
		return tr("Validating Access Token: Sending request.");
	case STATE_PROCESSING_RESPONSE:
		return tr("Validating Access Token: Processing response.");
	default:
		return YggdrasilTask::getStateMessage(state);
	}
}
