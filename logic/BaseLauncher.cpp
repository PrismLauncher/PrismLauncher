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

#include "BaseLauncher.h"
#include "MessageLevel.h"
#include <QDebug>
#include <QDir>
#include <QEventLoop>

BaseLauncher::BaseLauncher(InstancePtr instance): m_instance(instance)
{
}

void BaseLauncher::initializeEnvironment()
{
	// prepare the process environment
	QProcessEnvironment rawenv = QProcessEnvironment::systemEnvironment();

	QStringList ignored =
	{
		"JAVA_ARGS",
		"CLASSPATH",
		"CONFIGPATH",
		"JAVA_HOME",
		"JRE_HOME",
		"_JAVA_OPTIONS",
		"JAVA_OPTIONS",
		"JAVA_TOOL_OPTIONS"
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
#ifdef LINUX
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
		qDebug() << "Env: " << key << value;
		m_env.insert(key, value);
	}
#ifdef LINUX
	// HACK: Workaround for QTBUG-42500
	m_env.insert("LD_LIBRARY_PATH", "");
#endif

	// export some infos
	auto variables = getVariables();
	for (auto it = variables.begin(); it != variables.end(); ++it)
	{
		m_env.insert(it.key(), it.value());
	}
}

void BaseLauncher::init()
{
	initializeEnvironment();

	m_process.setProcessEnvironment(m_env);
	connect(&m_process, &LoggedProcess::log, this, &BaseLauncher::on_log);
	connect(&m_process, &LoggedProcess::stateChanged, this, &BaseLauncher::on_state);

	m_prelaunchprocess.setProcessEnvironment(m_env);
	connect(&m_prelaunchprocess, &LoggedProcess::log, this, &BaseLauncher::on_log);
	connect(&m_prelaunchprocess, &LoggedProcess::stateChanged, this, &BaseLauncher::on_pre_state);

	m_postlaunchprocess.setProcessEnvironment(m_env);
	connect(&m_postlaunchprocess, &LoggedProcess::log, this, &BaseLauncher::on_log);
	connect(&m_postlaunchprocess, &LoggedProcess::stateChanged, this, &BaseLauncher::on_post_state);

	// a process has been constructed for the instance. It is running from MultiMC POV
	m_instance->setRunning(true);
}


void BaseLauncher::setWorkdir(QString path)
{
	QDir mcDir(path);
	m_process.setWorkingDirectory(mcDir.absolutePath());
	m_prelaunchprocess.setWorkingDirectory(mcDir.absolutePath());
	m_postlaunchprocess.setWorkingDirectory(mcDir.absolutePath());
}

void BaseLauncher::printHeader()
{
	emit log(m_header);
}

void BaseLauncher::on_log(QStringList lines, MessageLevel::Enum level)
{
	logOutput(lines, level);
}

void BaseLauncher::logOutput(const QStringList &lines, MessageLevel::Enum defaultLevel)
{
	for (auto & line: lines)
	{
		logOutput(line, defaultLevel);
	}
}

void BaseLauncher::logOutput(QString line, MessageLevel::Enum level)
{
	// if the launcher part set a log level, use it
	auto innerLevel = MessageLevel::fromLine(line);
	if(innerLevel != MessageLevel::Unknown)
	{
		level = innerLevel;
	}

	// If the level is still undetermined, guess level
	if (level == MessageLevel::StdErr || level == MessageLevel::StdOut || level == MessageLevel::Unknown)
	{
		level = this->guessLevel(line, level);
	}

	// censor private user info
	line = censorPrivateInfo(line);

	emit log(line, level);
}

void BaseLauncher::preLaunch()
{
	QString prelaunch_cmd = m_instance->settings()->get("PreLaunchCommand").toString();
	if (!prelaunch_cmd.isEmpty())
	{
		prelaunch_cmd = substituteVariables(prelaunch_cmd);
		// Launch
		emit log(tr("Running Pre-Launch command: %1").arg(prelaunch_cmd));
		m_prelaunchprocess.start(prelaunch_cmd);
	}
	else
	{
		on_pre_state(LoggedProcess::Skipped);
	}
}

void BaseLauncher::on_pre_state(LoggedProcess::State state)
{
	switch(state)
	{
		case LoggedProcess::Aborted:
		case LoggedProcess::Crashed:
		case LoggedProcess::FailedToStart:
		{
			emit log(tr("Pre-Launch command failed with code %1.\n\n")
						 .arg(m_prelaunchprocess.exitCode()),
					 MessageLevel::Fatal);
			m_instance->cleanupAfterRun();
			emit prelaunch_failed(m_instance, m_prelaunchprocess.exitCode(), m_prelaunchprocess.exitStatus());
			// not running, failed
			m_instance->setRunning(false);
		}
		case LoggedProcess::Finished:
		{
			emit log(tr("Pre-Launch command ran successfully.\n\n"));
			m_instance->reload();
		}
		case LoggedProcess::Skipped:
		default:
			break;
	}
}

void BaseLauncher::on_state(LoggedProcess::State state)
{
	QProcess::ExitStatus estat = QProcess::NormalExit;
	switch(state)
	{
		case LoggedProcess::Aborted:
		case LoggedProcess::Crashed:
		case LoggedProcess::FailedToStart:
			estat = QProcess::CrashExit;
		case LoggedProcess::Finished:
		{
			auto exitCode = m_process.exitCode();
			m_postlaunchprocess.processEnvironment().insert("INST_EXITCODE", QString(exitCode));

			// run post-exit
			postLaunch();
			m_instance->cleanupAfterRun();
			// no longer running...
			m_instance->setRunning(false);
			emit ended(m_instance, exitCode, estat);
		}
		case LoggedProcess::Skipped:
			qWarning() << "Illegal game state: Skipped";
		default:
			break;
	}
}

void BaseLauncher::on_post_state(LoggedProcess::State state)
{
	switch(state)
	{
		case LoggedProcess::Aborted:
		case LoggedProcess::Crashed:
		case LoggedProcess::FailedToStart:
		case LoggedProcess::Finished:
		case LoggedProcess::Skipped:
		{

		}
		default:
			break;
	}
}

void BaseLauncher::killProcess()
{
	killed = true;
	if (m_prelaunchprocess.state() == LoggedProcess::Running)
	{
		m_prelaunchprocess.kill();
	}
	else if(m_process.state() == LoggedProcess::Running)
	{
		m_process.kill();
	}
	else if(m_postlaunchprocess.state() == LoggedProcess::Running)
	{
		m_postlaunchprocess.kill();
	}
}

void BaseLauncher::postLaunch()
{
	QString postlaunch_cmd = m_instance->settings()->get("PostExitCommand").toString();
	if (!postlaunch_cmd.isEmpty())
	{
		postlaunch_cmd = substituteVariables(postlaunch_cmd);
		emit log(tr("Running Post-Launch command: %1").arg(postlaunch_cmd));
		m_postlaunchprocess.start(postlaunch_cmd);
		if (m_postlaunchprocess.exitStatus() != QProcess::NormalExit)
		{
			emit log(tr("Post-Launch command failed with code %1.\n\n")
						 .arg(m_postlaunchprocess.exitCode()),
					 MessageLevel::Error);
			emit postlaunch_failed(m_instance, m_postlaunchprocess.exitCode(),
								   m_postlaunchprocess.exitStatus());
			// not running, failed
			m_instance->setRunning(false);
		}
		else
			emit log(tr("Post-Launch command ran successfully.\n\n"));
	}
}

QString BaseLauncher::substituteVariables(const QString &cmd) const
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

qint64 BaseLauncher::pid()
{
#ifdef Q_OS_WIN
	struct _PROCESS_INFORMATION *procinfo = m_process.pid();
	return procinfo->dwProcessId;
#else
	return m_process.pid();
#endif
}
