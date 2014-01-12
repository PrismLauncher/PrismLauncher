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
#include "MultiMC.h"

#include "MinecraftProcess.h"

#include <QDataStream>
#include <QFile>
#include <QDir>
#include <QProcessEnvironment>

#include "BaseInstance.h"

#include "osutils.h"
#include "pathutils.h"
#include "cmdutils.h"

#define IBUS "@im=ibus"

// constructor
MinecraftProcess::MinecraftProcess(BaseInstance *inst) : m_instance(inst)
{
	connect(this, SIGNAL(finished(int, QProcess::ExitStatus)),
			SLOT(finish(int, QProcess::ExitStatus)));

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
	env.insert("INST_DIR", QDir(inst->instanceRoot()).absolutePath());

	this->setProcessEnvironment(env);
	m_prepostlaunchprocess.setProcessEnvironment(env);

	// std channels
	connect(this, SIGNAL(readyReadStandardError()), SLOT(on_stdErr()));
	connect(this, SIGNAL(readyReadStandardOutput()), SLOT(on_stdOut()));
}

void MinecraftProcess::setWorkdir(QString path)
{
	QDir mcDir(path);
	this->setWorkingDirectory(mcDir.absolutePath());
	m_prepostlaunchprocess.setWorkingDirectory(mcDir.absolutePath());
}

QString MinecraftProcess::censorPrivateInfo(QString in)
{
	if(!m_account)
		return in;

	QString sessionId = m_account->sessionId();
	QString accessToken = m_account->accessToken();
	QString clientToken = m_account->clientToken();
	in.replace(sessionId, "<SESSION ID>");
	in.replace(accessToken, "<ACCESS TOKEN>");
	in.replace(clientToken, "<CLIENT TOKEN>");
	auto profile = m_account->currentProfile();
	if(profile)
	{
		QString profileId = profile->id;
		QString profileName = profile->name;
		in.replace(profileId, "<PROFILE ID>");
		in.replace(profileName, "<PROFILE NAME>");
	}

	auto i = m_account->user().properties.begin();
	while (i != m_account->user().properties.end())
	{
		in.replace(i.value(), "<" + i.key().toUpper() + ">");
		++i;
	}

	return in;
}

// console window
void MinecraftProcess::on_stdErr()
{
	QByteArray data = readAllStandardError();
	QString str = m_err_leftover + QString::fromLocal8Bit(data);
	m_err_leftover.clear();
	QStringList lines = str.split("\n");
	bool complete = str.endsWith("\n");

	for (int i = 0; i < lines.size() - 1; i++)
	{
		QString &line = lines[i];
		emit log(censorPrivateInfo(line), getLevel(line, MessageLevel::Error));
	}
	if (!complete)
		m_err_leftover = lines.last();
}

void MinecraftProcess::on_stdOut()
{
	QByteArray data = readAllStandardOutput();
	QString str = m_out_leftover + QString::fromLocal8Bit(data);
	m_out_leftover.clear();
	QStringList lines = str.split("\n");
	bool complete = str.endsWith("\n");

	for (int i = 0; i < lines.size() - 1; i++)
	{
		QString &line = lines[i];
		emit log(censorPrivateInfo(line), getLevel(line, MessageLevel::Message));
	}
	if (!complete)
		m_out_leftover = lines.last();
}

// exit handler
void MinecraftProcess::finish(int code, ExitStatus status)
{
	if (!killed)
	{
		if (status == NormalExit)
		{
			//: Message displayed on instance exit
			emit log(tr("Minecraft exited with exitcode %1.").arg(code));
		}
		else
		{
			//: Message displayed on instance crashed
			emit log(tr("Minecraft crashed with exitcode %1.").arg(code));
		}
	}
	else
	{
		//: Message displayed after the instance exits due to kill request
		emit log(tr("Minecraft was killed by user."), MessageLevel::Error);
	}

	m_prepostlaunchprocess.processEnvironment().insert("INST_EXITCODE", QString(code));

	// run post-exit
	if (!m_instance->settings().get("PostExitCommand").toString().isEmpty())
	{
		m_prepostlaunchprocess.start(m_instance->settings().get("PostExitCommand").toString());
		m_prepostlaunchprocess.waitForFinished();
		if (m_prepostlaunchprocess.exitStatus() != NormalExit)
		{
			emit postlaunch_failed(m_instance, m_prepostlaunchprocess.exitCode(),
								   m_prepostlaunchprocess.exitStatus());
		}
	}
	m_instance->cleanupAfterRun();
	emit ended(m_instance, code, status);
}

void MinecraftProcess::killMinecraft()
{
	killed = true;
	kill();
}

void MinecraftProcess::launch()
{
	if (!m_instance->settings().get("PreLaunchCommand").toString().isEmpty())
	{
		m_prepostlaunchprocess.start(m_instance->settings().get("PreLaunchCommand").toString());
		m_prepostlaunchprocess.waitForFinished();
		if (m_prepostlaunchprocess.exitStatus() != NormalExit)
		{
			m_instance->cleanupAfterRun();
			emit prelaunch_failed(m_instance, m_prepostlaunchprocess.exitCode(),
								  m_prepostlaunchprocess.exitStatus());
			return;
		}
	}

	m_instance->setLastLaunch();
	auto &settings = m_instance->settings();

	//////////// java arguments ////////////
	QStringList args;
	{
		// custom args go first. we want to override them if we have our own here.
		args.append(m_instance->extraArguments());

		// OSX dock icon and name
		#ifdef OSX
		args << "-Xdock:icon=icon.png";
		args << QString("-Xdock:name=\"%1\"").arg(m_instance->windowTitle());
		#endif

		// HACK: Stupid hack for Intel drivers. See: https://mojang.atlassian.net/browse/MCL-767
		#ifdef Q_OS_WIN32
		args << QString("-XX:HeapDumpPath=MojangTricksIntelDriversForPerformance_javaw.exe_"
						"minecraft.exe.heapdump");
		#endif

		args << QString("-Xms%1m").arg(settings.get("MinMemAlloc").toInt());
		args << QString("-Xmx%1m").arg(settings.get("MaxMemAlloc").toInt());
		args << QString("-XX:PermSize=%1m").arg(settings.get("PermGen").toInt());
		if(!m_nativeFolder.isEmpty())
			args << QString("-Djava.library.path=%1").arg(m_nativeFolder);
		args << "-jar" << PathCombine(MMC->bin(), "jars", "NewLaunch.jar");
	}

	QString JavaPath = m_instance->settings().get("JavaPath").toString();
	emit log("MultiMC version: " + MMC->version().toString() + "\n\n");
	emit log("Minecraft folder is:\n" + workingDirectory() + "\n\n");
	emit log("Java path is:\n" + JavaPath + "\n\n");
	QString allArgs = args.join(", ");
	emit log("Java Arguments:\n[" + censorPrivateInfo(allArgs) + "]\n\n");

	// instantiate the launcher part
	start(JavaPath, args);
	if (!waitForStarted())
	{
		//: Error message displayed if instace can't start
		emit log(tr("Could not launch minecraft!"), MessageLevel::Error);
		m_instance->cleanupAfterRun();
		emit launch_failed(m_instance);
		return;
	}
	// send the launch script to the launcher part
	QByteArray bytes = launchScript.toUtf8();
	writeData(bytes.constData(), bytes.length());
}

MessageLevel::Enum MinecraftProcess::getLevel(const QString &line, MessageLevel::Enum level)
{
	if (line.contains("[INFO]") || line.contains("[CONFIG]") || line.contains("[FINE]") ||
		line.contains("[FINER]") || line.contains("[FINEST]"))
		level = MessageLevel::Message;
	if (line.contains("[SEVERE]") || line.contains("[STDERR]"))
		level = MessageLevel::Error;
	if (line.contains("[WARNING]"))
		level = MessageLevel::Warning;
	if (line.contains("Exception in thread") || line.contains("    at "))
		level = MessageLevel::Fatal;
	if (line.contains("[DEBUG]"))
		level = MessageLevel::Debug;
	return level;
}
