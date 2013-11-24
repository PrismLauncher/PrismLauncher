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

#include <iostream>

#include "InstanceLauncher.h"
#include "MultiMC.h"

#include "gui/ConsoleWindow.h"
#include "gui/dialogs/LoginDialog.h"
#include "gui/dialogs/ProgressDialog.h"

#include "logic/net/LoginTask.h"
#include "logic/MinecraftProcess.h"
#include "logic/lists/InstanceList.h"

InstanceLauncher::InstanceLauncher(QString instId) : QObject(), instId(instId)
{
}

void InstanceLauncher::onTerminated()
{
	std::cout << "Minecraft exited" << std::endl;
	MMC->quit();
}

void InstanceLauncher::onLoginComplete()
{
	// TODO: Fix this.
	/*
	LoginTask *task = (LoginTask *)QObject::sender();
	auto result = task->getResult();
	auto instance = MMC->instances()->getInstanceById(instId);
	proc = instance->prepareForLaunch(result);
	if (!proc)
	{
		// FIXME: report error
		return;
	}
	console = new ConsoleWindow(proc);
	connect(console, SIGNAL(isClosing()), this, SLOT(onTerminated()));

	proc->setLogin(result.username, result.session_id);
	proc->launch();
	*/
}

void InstanceLauncher::doLogin(const QString &errorMsg)
{
	LoginDialog *loginDlg = new LoginDialog(nullptr, errorMsg);
	loginDlg->exec();
	if (loginDlg->result() == QDialog::Accepted)
	{
		PasswordLogin uInfo{loginDlg->getUsername(), loginDlg->getPassword()};

		ProgressDialog *tDialog = new ProgressDialog(nullptr);
		LoginTask *loginTask = new LoginTask(uInfo, tDialog);
		connect(loginTask, SIGNAL(succeeded()), SLOT(onLoginComplete()), Qt::QueuedConnection);
		connect(loginTask, SIGNAL(failed(QString)), SLOT(doLogin(QString)),
				Qt::QueuedConnection);
		tDialog->exec(loginTask);
	}
	// onLoginComplete(LoginResponse("Offline","Offline", 1));
}

int InstanceLauncher::launch()
{
	std::cout << "Launching Instance '" << qPrintable(instId) << "'" << std::endl;
	auto instance = MMC->instances()->getInstanceById(instId);
	if (!instance)
	{
		std::cout << "Could not find instance requested. note that you have to specify the ID, "
					 "not the NAME" << std::endl;
		return 1;
	}

	std::cout << "Logging in..." << std::endl;
	doLogin("");

	return MMC->exec();
}
