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
#include <QRegularExpression>

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
	// IBus is a Linux IME framework. For some reason, it breaks MC?
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

	// Log prepost launch command output (can be disabled.)
	if (m_instance->settings().get("LogPrePostOutput").toBool())
	{
		connect(&m_prepostlaunchprocess, &QProcess::readyReadStandardError,
				this, &MinecraftProcess::on_prepost_stdErr);
		connect(&m_prepostlaunchprocess, &QProcess::readyReadStandardOutput,
				this, &MinecraftProcess::on_prepost_stdOut);
	}
}

void MinecraftProcess::setWorkdir(QString path)
{
	QDir mcDir(path);
	this->setWorkingDirectory(mcDir.absolutePath());
	m_prepostlaunchprocess.setWorkingDirectory(mcDir.absolutePath());
}

QString MinecraftProcess::censorPrivateInfo(QString in)
{
	if(!m_session)
		return in;

	if(m_session->session != "-")
		in.replace(m_session->session, "<SESSION ID>");
	in.replace(m_session->access_token, "<ACCESS TOKEN>");
	in.replace(m_session->client_token, "<CLIENT TOKEN>");
	in.replace(m_session->uuid, "<PROFILE ID>");
	in.replace(m_session->player_name, "<PROFILE NAME>");

	auto i = m_session->u.properties.begin();
	while (i != m_session->u.properties.end())
	{
		in.replace(i.value(), "<" + i.key().toUpper() + ">");
		++i;
	}

	return in;
}

// console window
MessageLevel::Enum MinecraftProcess::guessLevel(const QString &line, MessageLevel::Enum level)
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
	if(line.contains("overwriting existing"))
		level = MessageLevel::Fatal;
	return level;
}

MessageLevel::Enum MinecraftProcess::getLevel(const QString &levelName)
{
	if (levelName == "MultiMC")
		return MessageLevel::MultiMC;
	else if (levelName == "Debug")
		return MessageLevel::Debug;
	else if (levelName == "Info")
		return MessageLevel::Info;
	else if (levelName == "Message")
		return MessageLevel::Message;
	else if (levelName == "Warning")
		return MessageLevel::Warning;
	else if (levelName == "Error")
		return MessageLevel::Error;
	else if (levelName == "Fatal")
		return MessageLevel::Fatal;
	// Skip PrePost, it's not exposed to !![]!
	else
		return MessageLevel::Message;
}

void MinecraftProcess::logOutput(const QStringList &lines,
								 MessageLevel::Enum defaultLevel,
								 bool guessLevel, bool censor)
{
	for (int i = 0; i < lines.size(); ++i)
		logOutput(lines[i], defaultLevel, guessLevel, censor);
}

void MinecraftProcess::logOutput(QString line,
								 MessageLevel::Enum defaultLevel,
								 bool guessLevel, bool censor)
{
	MessageLevel::Enum level = defaultLevel;

	// Level prefix
	int endmark = line.indexOf("]!");
	if (line.startsWith("!![") && endmark != -1)
	{
		level = getLevel(line.left(endmark).mid(3));
		line = line.mid(endmark + 2);
	}
	// Guess level
	else if (guessLevel)
		level = this->guessLevel(line, defaultLevel);

	if (censor)
		line = censorPrivateInfo(line);

	emit log(line, level);
}

void MinecraftProcess::on_stdErr()
{
	QByteArray data = readAllStandardError();
	QString str = m_err_leftover + QString::fromLocal8Bit(data);

	QStringList lines = str.split("\n");
	m_err_leftover = lines.takeLast();

	logOutput(lines, MessageLevel::Error);
}

void MinecraftProcess::on_stdOut()
{
	QByteArray data = readAllStandardOutput();
	QString str = m_out_leftover + QString::fromLocal8Bit(data);

	QStringList lines = str.split("\n");
	m_out_leftover = lines.takeLast();

	logOutput(lines);
}

void MinecraftProcess::on_prepost_stdErr()
{
	QByteArray data = m_prepostlaunchprocess.readAllStandardError();
	QString str = m_err_leftover + QString::fromLocal8Bit(data);

	QStringList lines = str.split("\n");
	m_err_leftover = lines.takeLast();

	logOutput(lines, MessageLevel::PrePost, false, false);
}

void MinecraftProcess::on_prepost_stdOut()
{
	QByteArray data = m_prepostlaunchprocess.readAllStandardOutput();
	QString str = m_out_leftover + QString::fromLocal8Bit(data);

	QStringList lines = str.split("\n");
	m_out_leftover = lines.takeLast();

	logOutput(lines, MessageLevel::PrePost, false, false);
}

// exit handler
void MinecraftProcess::finish(int code, ExitStatus status)
{
	// Flush console window
	if (!m_err_leftover.isEmpty())
	{
		logOutput(m_err_leftover, MessageLevel::Error);
		m_err_leftover.clear();
	}
	if (!m_out_leftover.isEmpty())
	{
		logOutput(m_out_leftover);
		m_out_leftover.clear();
	}

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
	QString postlaunch_cmd = m_instance->settings().get("PostExitCommand").toString();
	if (!postlaunch_cmd.isEmpty())
	{
		emit log(tr("Running Post-Launch command: %1").arg(postlaunch_cmd));
		m_prepostlaunchprocess.start(postlaunch_cmd);
		m_prepostlaunchprocess.waitForFinished();
		// Flush console window
		if (!m_err_leftover.isEmpty())
		{
			logOutput(m_err_leftover, MessageLevel::PrePost);
			m_err_leftover.clear();
		}
		if (!m_out_leftover.isEmpty())
		{
			logOutput(m_out_leftover, MessageLevel::PrePost);
			m_out_leftover.clear();
		}
		if (m_prepostlaunchprocess.exitStatus() != NormalExit)
		{
			emit log(tr("Post-Launch command failed with code %1.\n\n").arg(m_prepostlaunchprocess.exitCode()),
					 MessageLevel::Error);
			emit postlaunch_failed(m_instance, m_prepostlaunchprocess.exitCode(),
								   m_prepostlaunchprocess.exitStatus());
		}
		else
			emit log(tr("Post-Launch command ran successfully.\n\n"));
	}
	m_instance->cleanupAfterRun();
	emit ended(m_instance, code, status);
}

void MinecraftProcess::killMinecraft()
{
	killed = true;
	kill();
}

void MinecraftProcess::arm()
{
	emit log("MultiMC version: " + MMC->version().toString() + "\n\n");
	emit log("Minecraft folder is:\n" + workingDirectory() + "\n\n");

	QString prelaunch_cmd = m_instance->settings().get("PreLaunchCommand").toString();
	if (!prelaunch_cmd.isEmpty())
	{
		// Launch
		emit log(tr("Running Pre-Launch command: %1").arg(prelaunch_cmd));
		m_prepostlaunchprocess.start(prelaunch_cmd);
		// Wait
		m_prepostlaunchprocess.waitForFinished();
		// Flush console window
		if (!m_err_leftover.isEmpty())
		{
			logOutput(m_err_leftover, MessageLevel::PrePost);
			m_err_leftover.clear();
		}
		if (!m_out_leftover.isEmpty())
		{
			logOutput(m_out_leftover, MessageLevel::PrePost);
			m_out_leftover.clear();
		}
		// Process return values
		if (m_prepostlaunchprocess.exitStatus() != NormalExit)
		{
			emit log(tr("Pre-Launch command failed with code %1.\n\n").arg(m_prepostlaunchprocess.exitCode()),
					 MessageLevel::Fatal);
			m_instance->cleanupAfterRun();
			emit prelaunch_failed(m_instance, m_prepostlaunchprocess.exitCode(),
								  m_prepostlaunchprocess.exitStatus());
			return;
		}
		else
			emit log(tr("Pre-Launch command ran successfully.\n\n"));
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

void MinecraftProcess::launch()
{
	QString launchString("launch\n");
	QByteArray bytes = launchString.toUtf8();
	writeData(bytes.constData(), bytes.length());
}

void MinecraftProcess::abort()
{
	QString launchString("abort\n");
	QByteArray bytes = launchString.toUtf8();
	writeData(bytes.constData(), bytes.length());
}
