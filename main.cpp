
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

#include "gui/mainwindow.h"
#include <QApplication>

#include "appsettings.h"
#include "data/loginresponse.h"

#include "data/plugin/pluginmanager.h"

#include "pathutils.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	app.setOrganizationName("Forkk");
	app.setApplicationName("MultiMC 5");
	
	settings = new AppSettings(&app);
	
	// Register meta types.
	qRegisterMetaType<LoginResponse>("LoginResponse");
	
	
	// Initialize plugins.
	PluginManager::get().loadPlugins(PathCombine(qApp->applicationDirPath(), "plugins"));
	PluginManager::get().initInstanceTypes();
	
	MainWindow mainWin;
	mainWin.show();
	
	return app.exec();
}
