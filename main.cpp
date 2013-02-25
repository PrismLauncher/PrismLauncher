/* Copyright 2013 MultiMC Contributors
 *
 * Authors: Andrew Okin
 *          Orochimarufan <orochimarufan.x3@gmail.com>
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

#include <QApplication>
#include <QDir>

#include "gui/mainwindow.h"
#include "gui/logindialog.h"
#include "gui/taskdialog.h"
#include "gui/consolewindow.h"

#include "data/appsettings.h"
#include "instancelist.h"
#include "data/loginresponse.h"
#include "tasks/logintask.h"
#include "data/minecraftprocess.h"

#include "data/plugin/pluginmanager.h"

#include "pathutils.h"
#include "cmdutils.h"

#include "config.h"

using namespace Util::Commandline;

// Commandline instance launcher
class InstanceLauncher : public QObject
{
	Q_OBJECT
private:
	InstanceList instances;
	QString instId;
	InstancePtr instance;
	MinecraftProcess *proc;
	ConsoleWindow *console;
public:
	InstanceLauncher(QString instId) : QObject(), instances(settings->get("InstanceDir").toString())
	{
		this->instId = instId;
	}
	
private:
	InstancePtr findInstance(QString instId)
	{
		QListIterator<InstancePtr> iter(instances);
		InstancePtr inst;
		while(iter.hasNext())
		{
			inst = iter.next();
			if (inst->id() == instId)
				break;
		}
		if (inst->id() != instId)
			return InstancePtr();
		else
			return iter.peekPrevious();
	}
	
private slots:
	void onTerminated()
	{
		std::cout << "Minecraft exited" << std::endl;
		QApplication::instance()->quit();
	}
	
	void onLoginComplete(LoginResponse response)
	{
		// TODO: console
		console = new ConsoleWindow();
		proc = new MinecraftProcess(instance, response.getUsername(), response.getSessionID(), console);
		//if (instance->getShowConsole())
		console->show();
		connect(proc, SIGNAL(ended()), SLOT(onTerminated()));
		proc->launch();
	}
	
	void doLogin(const QString &errorMsg)
	{
		LoginDialog* loginDlg = new LoginDialog(nullptr, errorMsg);
		if (loginDlg->exec())
		{
			UserInfo uInfo(loginDlg->getUsername(), loginDlg->getPassword());
			
			TaskDialog* tDialog = new TaskDialog(nullptr);
			LoginTask* loginTask = new LoginTask(uInfo, tDialog);
			connect(loginTask, SIGNAL(loginComplete(LoginResponse)),
					SLOT(onLoginComplete(LoginResponse)), Qt::QueuedConnection);
			connect(loginTask, SIGNAL(loginFailed(QString)),
					SLOT(doLogin(QString)), Qt::QueuedConnection);
			tDialog->exec(loginTask);
		}
		//onLoginComplete(LoginResponse("Offline","Offline", 1));
	}
	
public:
	int launch()
	{
		std::cout << "Loading Instances..." << std::endl;
		instances.loadList();
		
		std::cout << "Launching Instance '" << qPrintable(instId) << "'" << std::endl;
		instance = findInstance(instId);
		if (instance.isNull())
		{
			std::cout << "Could not find instance requested. note that you have to specify the ID, not the NAME" << std::endl;
			return 1;
		}
		
		std::cout << "Logging in..." << std::endl;
		doLogin("");
		
		return QApplication::instance()->exec();
	}
};

int main(int argc, char *argv[])
{
	// initialize Qt
	QApplication app(argc, argv);
	app.setOrganizationName("Forkk");
	app.setApplicationName("MultiMC 5");
	
	// Print app header
	std::cout << "MultiMC 5" << std::endl;
	std::cout << "(c) 2013 MultiMC Contributors" << std::endl << std::endl;
	
	// Commandline parsing
	Parser parser(FlagStyle::GNU, ArgumentStyle::SpaceAndEquals);
	
	// --help
	parser.addSwitch("help");
	parser.addShortOpt("help", 'h');
	parser.addDocumentation("help", "display this help and exit.");
	// --version
	parser.addSwitch("version");
	parser.addShortOpt("version", 'V');
	parser.addDocumentation("version", "display program version and exit.");
	// --dir
	parser.addOption("dir", app.applicationDirPath());
	parser.addShortOpt("dir", 'd');
	parser.addDocumentation("dir", "use the supplied directory as MultiMC root instead of the binary location (use '.' for current)");
	// --update
	parser.addOption("update");
	parser.addShortOpt("update", 'u');
	parser.addDocumentation("update", "replaces the given file with the running executable", "<path>");
	// --quietupdate
	parser.addSwitch("quietupdate");
	parser.addShortOpt("quietupdate", 'U');
	parser.addDocumentation("quietupdate", "doesn't restart MultiMC after installing updates");
	// --launch
	parser.addOption("launch");
	parser.addShortOpt("launch", 'l');
	parser.addDocumentation("launch", "tries to launch the given instance", "<inst>");
	
	// parse the arguments
	QHash<QString, QVariant> args;
	try
	{
		args = parser.parse(app.arguments());
	}
	catch(ParsingError e)
	{
		std::cerr << "CommandLineError: " << e.what() << std::endl;
		std::cerr << "Try '%1 -h' to get help on MultiMC's command line parameters." << std::endl;
		return 1;
	}
	
	// display help and exit
	if (args["help"].toBool())
	{
		std::cout << qPrintable(parser.compileHelp(app.arguments()[0]));
		return 0;
	}
	
	// display version and exit
	if (args["version"].toBool())
	{
		std::cout << "Version " << VERSION_STR << std::endl;
		std::cout << "Git " << GIT_COMMIT << std::endl;
		std::cout << "Tag: " << JENKINS_BUILD_TAG << " " << (ARCH==x64?"x86_64":"x86") << std::endl;
		return 0;
	}
	
	// update
	// Note: cwd is always the current executable path!
	if (!args["update"].isNull())
	{
		std::cout << "Performing MultiMC update: " << qPrintable(args["update"].toString()) << std::endl;
		QString cwd = QDir::currentPath();
		QDir::setCurrent(app.applicationDirPath());
		QFile file(app.applicationFilePath());
		file.copy(args["update"].toString());
		if(args["quietupdate"].toBool())
			return 0;
		QDir::setCurrent(cwd);
	}
	
	// change directory
	QDir::setCurrent(args["dir"].toString());
	
	// load settings
	settings = new AppSettings(&app);
	
	// Register meta types.
	qRegisterMetaType<LoginResponse>("LoginResponse");
	
	// Initialize plugins.
	PluginManager::get().loadPlugins(PathCombine(qApp->applicationDirPath(), "plugins"));
	PluginManager::get().initInstanceTypes();
	
	// launch instance.
	if (!args["launch"].isNull())
		return InstanceLauncher(args["launch"].toString()).launch();
	
	// show main window
	MainWindow mainWin;
	mainWin.show();
	
	// loop
	return app.exec();
}

#include "main.moc"
