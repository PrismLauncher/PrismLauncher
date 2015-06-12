/* Copyright 2013-2015 MultiMC Contributors
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

#include "BaseProcess.h"
#include <QDebug>
#include <QDir>
#include <QEventLoop>

#define IBUS "@im=ibus"

MessageLevel::Enum MessageLevel::getLevel(const QString& levelName)
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

BaseProcess::BaseProcess(InstancePtr instance):  QProcess(), m_instance(instance)
{
}

void BaseProcess::init()
{
	connect(this, SIGNAL(finished(int, QProcess::ExitStatus)),
			SLOT(finish(int, QProcess::ExitStatus)));

	// prepare the process environment
	QProcessEnvironment rawenv = QProcessEnvironment::systemEnvironment();

	QProcessEnvironment env;

	QStringList ignored =
	{
		"JAVA_ARGS",
		"CLASSPATH",
		"CONFIGPATH",
		"JAVA_HOME",
		"JRE_HOME",
		"_JAVA_OPTIONS",
		"JAVA_OPTIONS",
		"JAVA_TOOL_OPTIONS",
		"CDPATH"
	};
	for(auto key: rawenv.keys())
	{
		auto value = rawenv.value(key);
		// filter out dangerous java crap
		if(ignored.contains(key))
		{
			qDebug() << "Env: ignoring" << key << value;
			continue;
		}
		// filter MultiMC-related things
		if(key.startsWith("QT_"))
		{
			qDebug() << "Env: ignoring" << key << value;
			continue;
		}
#ifdef Q_OS_LINUX
		// Do not pass LD_* variables to java. They were intended for MultiMC
		if(key.startsWith("LD_"))
		{
			qDebug() << "Env: ignoring" << key << value;
			continue;
		}
		// Strip IBus
		// IBus is a Linux IME framework. For some reason, it breaks MC?
		if (key == "XMODIFIERS" && value.contains(IBUS))
		{
			QString save = value;
			value.replace(IBUS, "");
			qDebug() << "Env: stripped" << IBUS << "from" << save << ":" << value;
		}
#endif
		if(key == "GAME_PRELOAD")
		{
			env.insert("LD_PRELOAD", value);
			continue;
		}
		if(key == "GAME_LIBRARY_PATH")
		{
			env.insert("LD_LIBRARY_PATH", value);
			continue;
		}
		qDebug() << "Env: " << key << value;
		env.insert(key, value);
	}
#ifdef Q_OS_LINUX
	// HACK: Workaround for QTBUG-42500
	if(!env.contains("LD_LIBRARY_PATH"))
	{
		env.insert("LD_LIBRARY_PATH", "");
	}
#endif

	// export some infos
	auto variables = getVariables();
	for (auto it = variables.begin(); it != variables.end(); ++it)
	{
		env.insert(it.key(), it.value());
	}

	this->setProcessEnvironment(env);
	m_prepostlaunchprocess.setProcessEnvironment(env);

	// std channels
	connect(this, SIGNAL(readyReadStandardError()), SLOT(on_stdErr()));
	connect(this, SIGNAL(readyReadStandardOutput()), SLOT(on_stdOut()));

	// Log prepost launch command output (can be disabled.)
	if (m_instance->settings()->get("LogPrePostOutput").toBool())
	{
		connect(&m_prepostlaunchprocess, &QProcess::readyReadStandardError, this,
				&BaseProcess::on_prepost_stdErr);
		connect(&m_prepostlaunchprocess, &QProcess::readyReadStandardOutput, this,
				&BaseProcess::on_prepost_stdOut);
	}

	// a process has been constructed for the instance. It is running from MultiMC POV
	m_instance->setRunning(true);
}


void BaseProcess::setWorkdir(QString path)
{
	QDir mcDir(path);
	this->setWorkingDirectory(mcDir.absolutePath());
	m_prepostlaunchprocess.setWorkingDirectory(mcDir.absolutePath());
}

void BaseProcess::printHeader()
{
	emit log(m_header);
}


void BaseProcess::logOutput(const QStringList &lines, MessageLevel::Enum defaultLevel,
								 bool guessLevel, bool censor)
{
	for (int i = 0; i < lines.size(); ++i)
		logOutput(lines[i], defaultLevel, guessLevel, censor);
}

void BaseProcess::logOutput(QString line, MessageLevel::Enum defaultLevel, bool guessLevel,
								 bool censor)
{
	MessageLevel::Enum level = defaultLevel;

	// Level prefix
	int endmark = line.indexOf("]!");
	if (line.startsWith("!![") && endmark != -1)
	{
		level = MessageLevel::getLevel(line.left(endmark).mid(3));
		line = line.mid(endmark + 2);
	}
	// Guess level
	else if (guessLevel)
		level = this->guessLevel(line, defaultLevel);

	if (censor)
		line = censorPrivateInfo(line);

	emit log(line, level);
}

void BaseProcess::on_stdErr()
{
	QByteArray data = readAllStandardError();
	QString str = m_err_leftover + QString::fromLocal8Bit(data);

	str.remove('\r');
	QStringList lines = str.split("\n");
	m_err_leftover = lines.takeLast();

	logOutput(lines, MessageLevel::Error);
}

void BaseProcess::on_stdOut()
{
	QByteArray data = readAllStandardOutput();
	QString str = m_out_leftover + QString::fromLocal8Bit(data);

	str.remove('\r');
	QStringList lines = str.split("\n");
	m_out_leftover = lines.takeLast();

	logOutput(lines);
}

void BaseProcess::on_prepost_stdErr()
{
	QByteArray data = m_prepostlaunchprocess.readAllStandardError();
	QString str = m_err_leftover + QString::fromLocal8Bit(data);

	str.remove('\r');
	QStringList lines = str.split("\n");
	m_err_leftover = lines.takeLast();

	logOutput(lines, MessageLevel::PrePost, false, false);
}

void BaseProcess::on_prepost_stdOut()
{
	QByteArray data = m_prepostlaunchprocess.readAllStandardOutput();
	QString str = m_out_leftover + QString::fromLocal8Bit(data);

	str.remove('\r');
	QStringList lines = str.split("\n");
	m_out_leftover = lines.takeLast();

	logOutput(lines, MessageLevel::PrePost, false, false);
}

// exit handler
void BaseProcess::finish(int code, ExitStatus status)
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
			emit log(tr("Game exited with exitcode %1.").arg(code));
		}
		else
		{
			//: Message displayed on instance crashed
			if(code == -1)
				emit log(tr("Game crashed.").arg(code));
			else
				emit log(tr("Game crashed with exitcode %1.").arg(code));
		}
	}
	else
	{
		//: Message displayed after the instance exits due to kill request
		emit log(tr("Game was killed by user."), MessageLevel::Error);
	}

	m_prepostlaunchprocess.processEnvironment().insert("INST_EXITCODE", QString(code));

	// run post-exit
	postLaunch();
	m_instance->cleanupAfterRun();
	// no longer running...
	m_instance->setRunning(false);
	emit ended(m_instance, code, status);
}

void BaseProcess::killProcess()
{
	killed = true;
	if (m_prepostlaunchprocess.state() == QProcess::Running)
	{
		m_prepostlaunchprocess.kill();
	}
	else
	{
		kill();
	}
}

bool BaseProcess::preLaunch()
{
	QString prelaunch_cmd = m_instance->settings()->get("PreLaunchCommand").toString();
	if (!prelaunch_cmd.isEmpty())
	{
		prelaunch_cmd = substituteVariables(prelaunch_cmd);
		// Launch
		emit log(tr("Running Pre-Launch command: %1").arg(prelaunch_cmd));
		m_prepostlaunchprocess.start(prelaunch_cmd);
		if (!waitForPrePost())
		{
			emit log(tr("The command failed to start"), MessageLevel::Fatal);
			return false;
		}
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
			emit log(tr("Pre-Launch command failed with code %1.\n\n")
						 .arg(m_prepostlaunchprocess.exitCode()),
					 MessageLevel::Fatal);
			m_instance->cleanupAfterRun();
			emit prelaunch_failed(m_instance, m_prepostlaunchprocess.exitCode(),
								  m_prepostlaunchprocess.exitStatus());
			// not running, failed
			m_instance->setRunning(false);
			return false;
		}
		else
			emit log(tr("Pre-Launch command ran successfully.\n\n"));

		return m_instance->reload();
	}
	return true;
}
bool BaseProcess::postLaunch()
{
	QString postlaunch_cmd = m_instance->settings()->get("PostExitCommand").toString();
	if (!postlaunch_cmd.isEmpty())
	{
		postlaunch_cmd = substituteVariables(postlaunch_cmd);
		emit log(tr("Running Post-Launch command: %1").arg(postlaunch_cmd));
		m_prepostlaunchprocess.start(postlaunch_cmd);
		if (!waitForPrePost())
		{
			return false;
		}
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
			emit log(tr("Post-Launch command failed with code %1.\n\n")
						 .arg(m_prepostlaunchprocess.exitCode()),
					 MessageLevel::Error);
			emit postlaunch_failed(m_instance, m_prepostlaunchprocess.exitCode(),
								   m_prepostlaunchprocess.exitStatus());
			// not running, failed
			m_instance->setRunning(false);
		}
		else
			emit log(tr("Post-Launch command ran successfully.\n\n"));

		return m_instance->reload();
	}
	return true;
}

bool BaseProcess::waitForPrePost()
{
	if (!m_prepostlaunchprocess.waitForStarted())
		return false;
	QEventLoop eventLoop;
	auto finisher = [this, &eventLoop](QProcess::ProcessState state)
	{
		if (state == QProcess::NotRunning)
		{
			eventLoop.quit();
		}
	};
	auto connection = connect(&m_prepostlaunchprocess, &QProcess::stateChanged, finisher);
	int ret = eventLoop.exec();
	disconnect(connection);
	return ret == 0;
}

QString BaseProcess::substituteVariables(const QString &cmd) const
{
	QString out = cmd;
	auto variables = getVariables();
	for (auto it = variables.begin(); it != variables.end(); ++it)
	{
		out.replace("$" + it.key(), it.value());
	}
	auto env = QProcessEnvironment::systemEnvironment();
	for (auto var : env.keys())
	{
		out.replace("$" + var, env.value(var));
	}
	return out;
}
