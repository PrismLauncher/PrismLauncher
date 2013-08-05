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

#include "MinecraftProcess.h"

#include <QDataStream>
#include <QFile>
#include <QDir>
//#include <QImage>
#include <QProcessEnvironment>

#include "BaseInstance.h"

#include "osutils.h"
#include "pathutils.h"
#include "cmdutils.h"

#define IBUS "@im=ibus"

// constructor
MinecraftProcess::MinecraftProcess( BaseInstance* inst ) :
	m_instance(inst)
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
	
	// std channels
	connect(this, SIGNAL(readyReadStandardError()), SLOT(on_stdErr()));
	connect(this, SIGNAL(readyReadStandardOutput()), SLOT(on_stdOut()));
}

void MinecraftProcess::setMinecraftArguments ( QStringList args )
{
	m_args = args;
}

void MinecraftProcess::setMinecraftWorkdir ( QString path )
{
	QDir mcDir(path);
	this->setWorkingDirectory(mcDir.absolutePath());
	m_prepostlaunchprocess.setWorkingDirectory(mcDir.absolutePath());
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
	m_instance->cleanupAfterRun();
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
	
	emit log(QString("Minecraft folder is: '%1'").arg(workingDirectory()));
	QString JavaPath = m_instance->settings().get("JavaPath").toString();
	emit log(QString("Java path: '%1'").arg(JavaPath));
	emit log(QString("Arguments: '%1'").arg(m_args.join("' '")));
	start(JavaPath, m_args);
	if (!waitForStarted())
	{
		emit log("Could not launch minecraft!");
		return;
		//TODO: error handling
	}
}


