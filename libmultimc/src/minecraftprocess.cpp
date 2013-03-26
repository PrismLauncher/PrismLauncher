/* Copyright 2013 MultiMC Contributors
 *
 * Authors: Orochimarufan <orochimarufan.x3@gmail.com>
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

#include "minecraftprocess.h"

#include <QDataStream>
#include <QFile>
#include <QDir>
//#include <QImage>
#include <QProcessEnvironment>

#include "instance.h"

#include "osutils.h"
#include "pathutils.h"

#define LAUNCHER_FILE "MultiMCLauncher.jar"
#define IBUS "@im=ibus"

// commandline splitter
QStringList MinecraftProcess::splitArgs(QString args)
{
	QStringList argv;
	QString current;
	bool escape = false;
	QChar inquotes;
	for (int i=0; i<args.length(); i++)
	{
		QChar cchar = args.at(i);
		
		// \ escaped
		if (escape) {
			current += cchar;
			escape = false;
			// in "quotes"
		} else if (!inquotes.isNull()) {
			if (cchar == 0x5C)
				escape = true;
			else if (cchar == inquotes)
				inquotes = 0;
			else
				current += cchar;
			// otherwise
		} else {
			if (cchar == 0x20) {
				if (!current.isEmpty()) {
					argv << current;
					current.clear();
				}
			} else if (cchar == 0x22 || cchar == 0x27)
				inquotes = cchar;
			else
				current += cchar;
		}
	}
	if (!current.isEmpty())
		argv << current;
	return argv;
}

// prepare tools
inline void MinecraftProcess::extractIcon(InstancePtr inst, QString destination)
{
//	QImage(":/icons/instances/" + inst->iconKey()).save(destination);
}

inline void MinecraftProcess::extractLauncher(QString destination)
{
	QFile(":/launcher/launcher.jar").copy(destination);
}

void MinecraftProcess::prepare(InstancePtr inst)
{
	extractLauncher(PathCombine(inst->minecraftDir(), LAUNCHER_FILE));
	extractIcon(inst, PathCombine(inst->minecraftDir(), "icon.png"));
}

// constructor
MinecraftProcess::MinecraftProcess(InstancePtr inst, QString user, QString session) :
	m_instance(inst), m_user(user), m_session(session)
{
	connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(finish(int, QProcess::ExitStatus)));
	
	// prepare the process environment
	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	
#ifdef LINUX
	// Strip IBus
	if (env.value("XMODIFIERS").contains(IBUS))
		env.insert("XMODIFIERS", env.value("XMODIFIERS").replace(IBUS, ""));
#endif
	
	// export some infos
	env.insert("INST_NAME", inst->name());
	env.insert("INST_ID", inst->id());
	env.insert("INST_DIR", QDir(inst->rootDir()).absolutePath());
	
	this->setProcessEnvironment(env);
	m_prepostlaunchprocess.setProcessEnvironment(env);
	
	// set the cwd
	QDir mcDir(inst->minecraftDir());
	this->setWorkingDirectory(mcDir.absolutePath());
	m_prepostlaunchprocess.setWorkingDirectory(mcDir.absolutePath());
	
	// std channels
	connect(this, SIGNAL(readyReadStandardError()), SLOT(on_stdErr()));
	connect(this, SIGNAL(readyReadStandardOutput()), SLOT(on_stdOut()));
}

// console window
void MinecraftProcess::on_stdErr()
{
	QByteArray data = readAllStandardError();
	QString str = m_err_leftover + QString::fromLocal8Bit(data);
	m_err_leftover.clear();
	QStringList lines = str.split("\n");
	bool complete = str.endsWith("\n");
	
	for(int i = 0; i < lines.size() - 1; i++)
	{
		QString & line = lines[i];
		MessageLevel::Enum level = MessageLevel::Error;
		if(line.contains("[INFO]") || line.contains("[CONFIG]") || line.contains("[FINE]") || line.contains("[FINER]") || line.contains("[FINEST]") )
			level = MessageLevel::Message;
		if(line.contains("[SEVERE]") || line.contains("[WARNING]") || line.contains("[STDERR]"))
			level = MessageLevel::Error;
		emit log(lines[i].toLocal8Bit(), level);
	}
	if(!complete)
		m_err_leftover = lines.last();
}

void MinecraftProcess::on_stdOut()
{
	QByteArray data = readAllStandardOutput();
	QString str = m_out_leftover + QString::fromLocal8Bit(data);
	m_out_leftover.clear();
	QStringList lines = str.split("\n");
	bool complete = str.endsWith("\n");
	
	for(int i = 0; i < lines.size() - 1; i++)
	{
		QString & line = lines[i];
		emit log(lines[i].toLocal8Bit(), MessageLevel::Message);
	}
	if(!complete)
		m_out_leftover = lines.last();
}

// exit handler
void MinecraftProcess::finish(int code, ExitStatus status)
{
	if (status != NormalExit)
	{
		//TODO: error handling
	}
	
	emit log("Minecraft exited.");
	
	m_prepostlaunchprocess.processEnvironment().insert("INST_EXITCODE", QString(code));
	
	// run post-exit
	if (!m_instance->settings().get("PostExitCommand").toString().isEmpty())
	{
		m_prepostlaunchprocess.start(m_instance->settings().get("PostExitCommand").toString());
		m_prepostlaunchprocess.waitForFinished();
		if (m_prepostlaunchprocess.exitStatus() != NormalExit)
		{
			//TODO: error handling
		}
	}
	
//	if (m_console != nullptr)
//		m_console->setMayClose(true);
	
	emit ended();
}

void MinecraftProcess::launch()
{
	if (!m_instance->settings().get("PreLaunchCommand").toString().isEmpty())
	{
		m_prepostlaunchprocess.start(m_instance->settings().get("PreLaunchCommand").toString());
		m_prepostlaunchprocess.waitForFinished();
		if (m_prepostlaunchprocess.exitStatus() != NormalExit)
		{
			//TODO: error handling
			return;
		}
	}
	
	m_instance->setLastLaunch();
	
	prepare(m_instance);
	
	genArgs();
	
	emit log(QString("Minecraft folder is: '%1'").arg(workingDirectory()));
	QString JavaPath = m_instance->settings().get("JavaPath").toString();
	emit log(QString("Java path: '%1'").arg(JavaPath));
	emit log(QString("Arguments: '%1'").arg(m_arguments.join("' '")));
	start(JavaPath, m_arguments);
	if (!waitForStarted())
	{
		emit log("Could not launch minecraft!");
		return;
		//TODO: error handling
	}
	
//	if(m_console != nullptr)
//		m_console->setMayClose(false);
}

void MinecraftProcess::genArgs()
{
	// start fresh
	m_arguments.clear();
	
	// window size
	QString windowSize;
	if (m_instance->settings().get("LaunchMaximized").toBool())
		windowSize = "max";
	else
		windowSize = QString("%1x%2").
				arg(m_instance->settings().get("MinecraftWinWidth").toInt()).
				arg(m_instance->settings().get("MinecraftWinHeight").toInt());
	
	// window title
	QString windowTitle;
	windowTitle.append("MultiMC: ").append(m_instance->name());
	
	// Java arguments
	m_arguments.append(splitArgs(m_instance->settings().get("JvmArgs").toString()));
	
#ifdef OSX
	// OSX dock icon and name
	m_arguments << "-Xdock:icon=icon.png";
	m_arguments << QString("-Xdock:name=\"%1\"").arg(windowTitle);
#endif
	
	// lwjgl
	QString lwjgl = m_instance->lwjglVersion();
	if (lwjgl != "Mojang")
		lwjgl = QDir(globalSettings->get("LWJGLDir").toString() + "/" + lwjgl).absolutePath();
	
	// launcher arguments
	m_arguments << QString("-Xms%1m").arg(m_instance->settings().get("MinMemAlloc").toInt());
	m_arguments << QString("-Xmx%1m").arg(m_instance->settings().get("MaxMemAlloc").toInt());
	m_arguments << "-jar" << LAUNCHER_FILE;
	m_arguments << m_user;
	m_arguments << m_session;
	m_arguments << windowTitle;
	m_arguments << windowSize;
	m_arguments << lwjgl;
}
