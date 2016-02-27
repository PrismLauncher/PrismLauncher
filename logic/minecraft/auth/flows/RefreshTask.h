/* Copyright 2013-2015 MultiMC Contributors
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

#include "../YggdrasilTask.h"

#include <QObject>
#include <QString>
#include <QJsonObject>

/**
 * The authenticate task takes a MojangAccount with a possibly timed-out access token
 * and attempts to authenticate with Mojang's servers.
 * If successful, it will set the new access token. The token is considered validated.
 */
class RefreshTask : public YggdrasilTask
{
	Q_OBJECT
public:
	RefreshTask(MojangAccount * account);

protected:
	virtual QJsonObject getRequestContent() const override;

	virtual QString getEndpoint() const override;

	virtual void processResponse(QJsonObject responseData) override;

	virtual QString getStateMessage() const override;
};

