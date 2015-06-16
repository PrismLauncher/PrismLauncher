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
#include "MMCStrings.h"
#include "java/JavaChecker.h"
#include <pathutils.h>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QStandardPaths>

#define IBUS "@im=ibus"

BaseLauncher* BaseLauncher::create(MinecraftInstancePtr inst)
{
	auto proc = new BaseLauncher(inst);
	proc->init();
	return proc;
}


BaseLauncher::BaseLauncher(InstancePtr instance): m_instance(instance)
{
}

QString BaseLauncher::censorPrivateInfo(QString in)
{
	if (!m_session)
		return in;

	if (m_session->session != "-")
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
MessageLevel::Enum BaseLauncher::guessLevel(const QString &line, MessageLevel::Enum level)
{
	QRegularExpression re("\\[(?<timestamp>[0-9:]+)\\] \\[[^/]+/(?<level>[^\\]]+)\\]");
	auto match = re.match(line);
	if(match.hasMatch())
	{
		// New style logs from log4j
		QString timestamp = match.captured("timestamp");
		QString levelStr = match.captured("level");
		if(levelStr == "INFO")
			level = MessageLevel::Message;
		if(levelStr == "WARN")
			level = MessageLevel::Warning;
		if(levelStr == "ERROR")
			level = MessageLevel::Error;
		if(levelStr == "FATAL")
			level = MessageLevel::Fatal;
		if(levelStr == "TRACE" || levelStr == "DEBUG")
			level = MessageLevel::Debug;
	}
	else
	{
		// Old style forge logs
		if (line.contains("[INFO]") || line.contains("[CONFIG]") || line.contains("[FINE]") ||
			line.contains("[FINER]") || line.contains("[FINEST]"))
			level = MessageLevel::Message;
		if (line.contains("[SEVERE]") || line.contains("[STDERR]"))
			level = MessageLevel::Error;
		if (line.contains("[WARNING]"))
			level = MessageLevel::Warning;
		if (line.contains("[DEBUG]"))
			level = MessageLevel::Debug;
	}
	if (line.contains("overwriting existing"))
		return MessageLevel::Fatal;
	if (line.contains("Exception in thread") || line.contains(QRegularExpression("\\s+at ")))
		return MessageLevel::Error;
	return level;
}

QMap<QString, QString> BaseLauncher::getVariables() const
{
	auto mcInstance = std::dynamic_pointer_cast<MinecraftInstance>(m_instance);
	QMap<QString, QString> out;
	out.insert("INST_NAME", mcInstance->name());
	out.insert("INST_ID", mcInstance->id());
	out.insert("INST_DIR", QDir(mcInstance->instanceRoot()).absolutePath());
	out.insert("INST_MC_DIR", QDir(mcInstance->minecraftRoot()).absolutePath());
	out.insert("INST_JAVA", mcInstance->settings()->get("JavaPath").toString());
	out.insert("INST_JAVA_ARGS", javaArguments().join(' '));
	return out;
}

QStringList BaseLauncher::javaArguments() const
{
	QStringList args;

	// custom args go first. we want to override them if we have our own here.
	args.append(m_instance->extraArguments());

	// OSX dock icon and name
#ifdef Q_OS_MAC
	args << "-Xdock:icon=icon.png";
	args << QString("-Xdock:name=\"%1\"").arg(m_instance->windowTitle());
#endif

	// HACK: Stupid hack for Intel drivers. See: https://mojang.atlassian.net/browse/MCL-767
#ifdef Q_OS_WIN32
	args << QString("-XX:HeapDumpPath=MojangTricksIntelDriversForPerformance_javaw.exe_"
					"minecraft.exe.heapdump");
#endif

	args << QString("-Xms%1m").arg(m_instance->settings()->get("MinMemAlloc").toInt());
	args << QString("-Xmx%1m").arg(m_instance->settings()->get("MaxMemAlloc").toInt());

	// No PermGen in newer java.
	auto javaVersion = m_instance->settings()->get("JavaVersion");
	if(Strings::naturalCompare(javaVersion.toString(), "1.8.0", Qt::CaseInsensitive) < 0)
	{
		auto permgen = m_instance->settings()->get("PermGen").toInt();
		if (permgen != 64)
		{
			args << QString("-XX:PermSize=%1m").arg(permgen);
		}
	}

	args << "-Duser.language=en";
	if (!m_nativeFolder.isEmpty())
		args << QString("-Djava.library.path=%1").arg(m_nativeFolder);
	args << "-jar" << PathCombine(QCoreApplication::applicationDirPath(), "jars", "NewLaunch.jar");

	return args;
}

bool BaseLauncher::checkJava(QString JavaPath)
{
	auto realJavaPath = QStandardPaths::findExecutable(JavaPath);
	if (realJavaPath.isEmpty())
	{
		emit log(tr("The java binary \"%1\" couldn't be found. You may have to set up java "
					"if Minecraft fails to launch.").arg(JavaPath),
				 MessageLevel::Warning);
	}

	QFileInfo javaInfo(realJavaPath);
	qlonglong javaUnixTime = javaInfo.lastModified().toMSecsSinceEpoch();
	auto storedUnixTime = m_instance->settings()->get("JavaTimestamp").toLongLong();
	// if they are not the same, check!
	if(javaUnixTime != storedUnixTime)
	{
		QEventLoop ev;
		auto checker = std::make_shared<JavaChecker>();
		bool successful = false;
		QString errorLog;
		QString version;
		emit log(tr("Checking Java version..."), MessageLevel::MultiMC);
		connect(checker.get(), &JavaChecker::checkFinished,
				[&](JavaCheckResult result)
				{
					successful = result.valid;
					errorLog = result.errorLog;
					version = result.javaVersion;
					ev.exit();
				});
		checker->m_path = realJavaPath;
		checker->performCheck();
		ev.exec();
		if(!successful)
		{
			// Error message displayed if java can't start
			emit log(tr("Could not start java:"), MessageLevel::Error);
			auto lines = errorLog.split('\n');
			for(auto line: lines)
			{
				emit log(line, MessageLevel::Error);
			}
			emit log("\nCheck your MultiMC Java settings.", MessageLevel::MultiMC);
			m_instance->cleanupAfterRun();
			emit launch_failed(m_instance);
			// not running, failed
			m_instance->setRunning(false);
			return false;
		}
		emit log(tr("Java version is %1!\n").arg(version), MessageLevel::MultiMC);
		m_instance->settings()->set("JavaVersion", version);
		m_instance->settings()->set("JavaTimestamp", javaUnixTime);
	}
	return true;
}

void BaseLauncher::arm()
{
	printHeader();
	emit log("Minecraft folder is:\n" + m_process.workingDirectory() + "\n\n");

	/*
	if (!preLaunch())
	{
		emit ended(m_instance, 1, QProcess::CrashExit);
		return;
	}
	*/

	m_instance->setLastLaunch();

	QString JavaPath = m_instance->settings()->get("JavaPath").toString();
	emit log("Java path is:\n" + JavaPath + "\n\n");

	if(!checkJava(JavaPath))
	{
		return;
	}

	QStringList args = javaArguments();
	QString allArgs = args.join(", ");
	emit log("Java Arguments:\n[" + censorPrivateInfo(allArgs) + "]\n\n");

	QString wrapperCommand = m_instance->settings()->get("WrapperCommand").toString();
	if(!wrapperCommand.isEmpty())
	{
		auto realWrapperCommand = QStandardPaths::findExecutable(wrapperCommand);
		if (realWrapperCommand.isEmpty())
		{
			emit log(tr("The wrapper command \"%1\" couldn't be found.").arg(wrapperCommand), MessageLevel::Warning);
			m_instance->cleanupAfterRun();
			emit launch_failed(m_instance);
			m_instance->setRunning(false);
			return;
		}
		emit log("Wrapper command is:\n" + wrapperCommand + "\n\n");
		args.prepend(JavaPath);
		m_process.start(wrapperCommand, args);
	}
	else
	{
		m_process.start(JavaPath, args);
	}

	// instantiate the launcher part
	if (!m_process.waitForStarted())
	{
		//: Error message displayed if instace can't start
		emit log(tr("Could not launch minecraft!"), MessageLevel::Error);
		m_instance->cleanupAfterRun();
		emit launch_failed(m_instance);
		// not running, failed
		m_instance->setRunning(false);
		return;
	}

	emit log(tr("Minecraft process ID: %1\n\n").arg(m_process.processId()), MessageLevel::MultiMC);

	// send the launch script to the launcher part
	m_process.write(launchScript.toUtf8());
}

void BaseLauncher::launch()
{
	QString launchString("launch\n");
	m_process.write(launchString.toUtf8());
}

void BaseLauncher::abort()
{
	QString launchString("abort\n");
	m_process.write(launchString.toUtf8());
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
		if(key == "GAME_PRELOAD")
		{
			m_env.insert("LD_PRELOAD", value);
			continue;
		}
		if(key == "GAME_LIBRARY_PATH")
		{
			m_env.insert("LD_LIBRARY_PATH", value);
			continue;
		}
#endif
		qDebug() << "Env: " << key << value;
		m_env.insert(key, value);
	}
#ifdef Q_OS_LINUX
	// HACK: Workaround for QTBUG42500
	if(!m_env.contains("LD_LIBRARY_PATH"))
	{
		m_env.insert("LD_LIBRARY_PATH", "");
	}
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
