#include "InstanceLauncher.h"
#include "MultiMC.h"

#include <iostream>
#include "gui/logindialog.h"
#include "gui/ProgressDialog.h"
#include "gui/consolewindow.h"
#include "logic/tasks/LoginTask.h"
#include "logic/MinecraftProcess.h"
#include "lists/InstanceList.h"


InstanceLauncher::InstanceLauncher ( QString instId )
	:QObject(), instId ( instId )
{}

void InstanceLauncher::onTerminated()
{
	std::cout << "Minecraft exited" << std::endl;
	MMC->quit();
}

void InstanceLauncher::onLoginComplete()
{
	LoginTask * task = ( LoginTask * ) QObject::sender();
	auto result = task->getResult();
	auto instance = MMC->instances()->getInstanceById(instId);
	proc = instance->prepareForLaunch ( result.username, result.sessionID );
	if ( !proc )
	{
		//FIXME: report error
		return;
	}
	console = new ConsoleWindow(proc);
	console->show();

	connect ( proc, SIGNAL ( ended() ), SLOT ( onTerminated() ) );
	connect ( proc, SIGNAL ( log ( QString,MessageLevel::Enum ) ), console, SLOT ( write ( QString,MessageLevel::Enum ) ) );

	proc->launch();
}

void InstanceLauncher::doLogin ( const QString& errorMsg )
{
	LoginDialog* loginDlg = new LoginDialog ( nullptr, errorMsg );
	loginDlg->exec();
	if ( loginDlg->result() == QDialog::Accepted )
	{
		UserInfo uInfo {loginDlg->getUsername(), loginDlg->getPassword() };

		ProgressDialog* tDialog = new ProgressDialog ( nullptr );
		LoginTask* loginTask = new LoginTask ( uInfo, tDialog );
		connect ( loginTask, SIGNAL ( succeeded() ),SLOT ( onLoginComplete() ), Qt::QueuedConnection );
		connect ( loginTask, SIGNAL ( failed ( QString ) ),SLOT ( doLogin ( QString ) ), Qt::QueuedConnection );
		tDialog->exec ( loginTask );
	}
	//onLoginComplete(LoginResponse("Offline","Offline", 1));
}

int InstanceLauncher::launch()
{
	std::cout << "Launching Instance '" << qPrintable ( instId ) << "'" << std::endl;
	auto instance = MMC->instances()->getInstanceById(instId);
	if ( instance.isNull() )
	{
		std::cout << "Could not find instance requested. note that you have to specify the ID, not the NAME" << std::endl;
		return 1;
	}

	std::cout << "Logging in..." << std::endl;
	doLogin ( "" );

	return MMC->exec();
}
