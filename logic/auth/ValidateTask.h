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

#include <logic/auth/YggdrasilTask.h>

#include <QObject>
#include <QString>
#include <QJsonObject>

/**
 * The validate task takes a MojangAccount and checks to make sure its access token is valid.
 */
class ValidateTask : public YggdrasilTask
{
Q_OBJECT
public:
	ValidateTask(MojangAccountPtr account, QObject* parent=0);

protected:
	virtual QJsonObject getRequestContent() const;
	
	virtual QString getEndpoint() const;

	virtual bool processResponse(QJsonObject responseData);
	
	QString getStateMessage(const YggdrasilTask::State state) const;

private:
};

