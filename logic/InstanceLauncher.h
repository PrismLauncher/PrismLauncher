/* Copyright 2013-2014 MultiMC Contributors
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

#include <QObject>

class MinecraftProcess;
class ConsoleWindow;

// Commandline instance launcher
class InstanceLauncher : public QObject
{
	Q_OBJECT

private:
	QString instId;
	MinecraftProcess *proc;
	ConsoleWindow *console;

public:
	InstanceLauncher(QString instId);

private
slots:
	void onTerminated();
	void onLoginComplete();
	void doLogin(const QString &errorMsg);

public:
	int launch();
};
