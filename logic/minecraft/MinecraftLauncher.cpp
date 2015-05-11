/* Copyright 2013-2014 MultiMC Contributors
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
#include "minecraft/MinecraftLauncher.h"
#include "BaseInstance.h"
#include <java/JavaChecker.h>
#include <MMCStrings.h>

#include <QDataStream>
#include <QFile>
#include <QDir>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QCoreApplication>

#include "osutils.h"
#include "pathutils.h"
#include "cmdutils.h"

// constructor
MinecraftLauncher::MinecraftLauncher(MinecraftInstancePtr inst) : BaseLauncher(inst)
{
}

MinecraftLauncher* MinecraftLauncher::create(MinecraftInstancePtr inst)
{
	auto proc = new MinecraftLauncher(inst);
	proc->init();
	return proc;
}


QString MinecraftLauncher::censorPrivateInfo(QString in)
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
MessageLevel::Enum MinecraftLauncher::guessLevel(const QString &line, MessageLevel::Enum level)
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

QMap<QString, QString> MinecraftLauncher::getVariables() const
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

QStringList MinecraftLauncher::javaArguments() const
{
	QStringList args;

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

bool MinecraftLauncher::checkJava(QString JavaPath)
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

void MinecraftLauncher::arm()
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

void MinecraftLauncher::launch()
{
	QString launchString("launch\n");
	m_process.write(launchString.toUtf8());
}

void MinecraftLauncher::abort()
{
	QString launchString("abort\n");
	m_process.write(launchString.toUtf8());
}
