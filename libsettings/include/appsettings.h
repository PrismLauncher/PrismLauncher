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

#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QObject>
#include <QSettings>
//#include <QColor>
#include <QPoint>

#include <apputils.h>
#include <osutils.h>

#include "libsettings_config.h"

#if WINDOWS
#define JPATHKEY "JavaPathWindows"
#elif OSX
#define JPATHKEY "JavaPathOSX"
#else
#define JPATHKEY "JavaPathLinux"
#endif

#define DEFINE_SETTING_ADVANCED(funcName, name, valType, defVal) \
	virtual valType get ## funcName() const { return getValue(name, defVal).value<valType>(); } \
	virtual void set ## funcName(valType value) { setValue(name, value); }

#define DEFINE_SETTING(name, valType, defVal) \
	DEFINE_SETTING_ADVANCED(name, STR_VAL(name), valType, defVal)


class LIBMMCSETTINGS_EXPORT SettingsBase : public QObject
{
	Q_OBJECT
public:
	explicit SettingsBase(QObject *parent = 0);
	
	// Updates
	DEFINE_SETTING(UseDevBuilds, bool, false)
	DEFINE_SETTING(AutoUpdate, bool, true)
	
	// Folders
	DEFINE_SETTING(InstanceDir, QString, "instances")
	DEFINE_SETTING(CentralModsDir, QString, "mods")
	DEFINE_SETTING(LWJGLDir, QString, "lwjgl")
	
	// Console
	DEFINE_SETTING(ShowConsole, bool, true)
	DEFINE_SETTING(AutoCloseConsole, bool, true)
	
	// Toolbar settings
	DEFINE_SETTING(InstanceToolbarVisible, bool, true)
	DEFINE_SETTING(InstanceToolbarPosition, QPoint, QPoint())
	
	// Console Colors
	// Currently commented out because QColor is a part of QtGUI
//	DEFINE_SETTING(SysMessageColor, QColor, QColor(Qt::blue))
//	DEFINE_SETTING(StdOutColor, QColor, QColor(Qt::black))
//	DEFINE_SETTING(StdErrColor, QColor, QColor(Qt::red))
	
	// Window Size
	DEFINE_SETTING(LaunchCompatMode, bool, false)
	DEFINE_SETTING(LaunchMaximized, bool, false)
	DEFINE_SETTING(MinecraftWinWidth, int, 854)
	DEFINE_SETTING(MinecraftWinHeight, int, 480)
	
	// Auto login
	DEFINE_SETTING(AutoLogin, bool, false)
	
	// Memory
	DEFINE_SETTING(MinMemAlloc, int, 512)
	DEFINE_SETTING(MaxMemAlloc, int, 1024)
	
	// Java Settings
	DEFINE_SETTING_ADVANCED(JavaPath, JPATHKEY, QString, "java")
	DEFINE_SETTING(JvmArgs, QString, "")
	
	// Custom Commands
	DEFINE_SETTING(PreLaunchCommand, QString, "")
	DEFINE_SETTING(PostExitCommand, QString, "")
	
	virtual QVariant getValue(const QString& name, QVariant defVal = QVariant()) const = 0;
	virtual void setValue(const QString& name, QVariant val) = 0;
};

class LIBMMCSETTINGS_EXPORT AppSettings : public SettingsBase
{
	Q_OBJECT
public:
	explicit AppSettings(QObject *parent = 0);
	
	QSettings& getConfig() { return config; }
	
	virtual QVariant getValue(const QString &name, QVariant defVal = QVariant()) const;
	virtual void setValue(const QString& name, QVariant val);

protected:
	QSettings config;
};

#undef DEFINE_SETTING_ADVANCED
#undef DEFINE_SETTING

LIBMMCSETTINGS_EXPORT extern AppSettings* settings;

#endif // APPSETTINGS_H
