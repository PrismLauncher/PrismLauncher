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

#include "logintask.h"

LoginTask::LoginTask(const UserInfo &uInfo, QObject *parent) :
	Task(parent), uInfo(uInfo)
{
	
}

void LoginTask::executeTask()
{
	setStatus("Logging in...");
	
	// TODO: PLACEHOLDER
	for (int p = 0; p < 100; p++)
	{
		msleep(25);
		setProgress(p);
	}
	
	if (uInfo.getUsername() == "test")
	{
		LoginResponse response("test", "Fake Session ID");
		emit loginComplete(response);
	}
	else
	{
		emit loginFailed("Testing");
	}
}
