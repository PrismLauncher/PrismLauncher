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

#include "appsettings.h"

#include <setting.h>

#include <QPoint>
#include <QApplication>
//#include <QColor>

AppSettings::AppSettings(QObject *parent) :
	INISettingsObject(QApplication::applicationDirPath() + "/multimc.cfg",parent)
{
	// Updates
	registerSetting(new Setting("UseDevBuilds", false));
	registerSetting(new Setting("AutoUpdate", true));
	
	// Folders
	registerSetting(new Setting("InstanceDir", "instances"));
	registerSetting(new Setting("CentralModsDir", "mods"));
	registerSetting(new Setting("LWJGLDir", "lwjgl"));
	
	// Console
	registerSetting(new Setting("ShowConsole", true));
	registerSetting(new Setting("AutoCloseConsole", true));
	
	// Toolbar settings
	registerSetting(new Setting("InstanceToolbarVisible", true));
	registerSetting(new Setting("InstanceToolbarPosition", QPoint()));
	
	// Console Colors
//	registerSetting(new Setting("SysMessageColor", QColor(Qt::blue)));
//	registerSetting(new Setting("StdOutColor", QColor(Qt::black)));
//	registerSetting(new Setting("StdErrColor", QColor(Qt::red)));
	
	// Window Size
	registerSetting(new Setting("LaunchCompatMode", false));
	registerSetting(new Setting("LaunchMaximized", false));
	registerSetting(new Setting("MinecraftWinWidth", 854));
	registerSetting(new Setting("MinecraftWinHeight", 480));
	
	// Auto login
	registerSetting(new Setting("AutoLogin", false));
	
	// Memory
	registerSetting(new Setting("MinMemAlloc", 512));
	registerSetting(new Setting("MaxMemAlloc", 1024));
	
	// Java Settings
	registerSetting(new Setting("JavaPath", "java"));
	registerSetting(new Setting("JvmArgs", ""));
	
	// Custom Commands
	registerSetting(new Setting("PreLaunchCommand", ""));
	registerSetting(new Setting("PostExitCommand", ""));
}
