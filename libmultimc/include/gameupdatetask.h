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

#ifndef GAMEUPDATETASK_H
#define GAMEUPDATETASK_H

#include <QObject>

#include "loginresponse.h"

#include "libmmc_config.h"

/*!
 * \brief The game update task is the task that handles downloading instances.
 * Each instance type has its own class inheriting from this base game update task.
 */
class LIBMULTIMC_EXPORT GameUpdateTask : public QObject
{
	Q_OBJECT
public:
	explicit GameUpdateTask(const LoginResponse &response, QObject *parent = 0);
	
signals:
	/*!
	 * \brief Signal emitted when the game update is complete.
	 * \param response The login response received from login task.
	 */
	void gameUpdateComplete(const LoginResponse &response);
	
	/*!
	 * \brief Signal emitted if the game update fails.
	 * \param errorMsg An error message to be displayed to the user.
	 */
	void gameUpdateFailed(const QString &errorMsg);
	
private:
	LoginResponse m_response;
};

#endif // GAMEUPDATETASK_H
