/* Copyright 2013-2015 MultiMC Contributors
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

#include "CheckJava.h"
#include <launch/LaunchTask.h>
#include <FileSystem.h>
#include <QStandardPaths>
#include <QFileInfo>
#include <sys.h>

void CheckJava::executeTask()
{
	auto instance = m_parent->instance();
	auto settings = instance->settings();
	m_javaPath = FS::ResolveExecutable(settings->get("JavaPath").toString());
	bool perInstance = settings->get("OverrideJava").toBool() || settings->get("OverrideJavaLocation").toBool();

	auto realJavaPath = QStandardPaths::findExecutable(m_javaPath);
	if (realJavaPath.isEmpty())
	{
		if (perInstance)
		{
			emit logLine(
				tr("The java binary \"%1\" couldn't be found. Please fix the java path "
				   "override in the instance's settings or disable it.").arg(m_javaPath),
				MessageLevel::Warning);
		}
		else
		{
			emit logLine(tr("The java binary \"%1\" couldn't be found. Please set up java in "
							"the settings.").arg(m_javaPath),
						 MessageLevel::Warning);
		}
		emitFailed(tr("Java path is not valid."));
		return;
	}
	else
	{
		emit logLine("Java path is:\n" + m_javaPath + "\n\n", MessageLevel::MultiMC);
	}

	QFileInfo javaInfo(realJavaPath);
	qlonglong javaUnixTime = javaInfo.lastModified().toMSecsSinceEpoch();
	auto storedUnixTime = settings->get("JavaTimestamp").toLongLong();
	auto storedArchitecture = settings->get("JavaArchitecture").toString();
	auto storedVersion = settings->get("JavaVersion").toString();
	m_javaUnixTime = javaUnixTime;
	// if timestamps are not the same, or something is missing, check!
	if (javaUnixTime != storedUnixTime || storedVersion.size() == 0 || storedArchitecture.size() == 0)
	{
		m_JavaChecker = std::make_shared<JavaChecker>();
		emit logLine(tr("Checking Java version..."), MessageLevel::MultiMC);
		connect(m_JavaChecker.get(), &JavaChecker::checkFinished, this, &CheckJava::checkJavaFinished);
		m_JavaChecker->m_path = realJavaPath;
		m_JavaChecker->performCheck();
		return;
	}
	else
	{
		auto verString = instance->settings()->get("JavaVersion").toString();
		auto archString = instance->settings()->get("JavaArchitecture").toString();
		printJavaInfo(verString, archString);
	}
	emitSucceeded();
}

void CheckJava::checkJavaFinished(JavaCheckResult result)
{
	if (!result.valid)
	{
		// Error message displayed if java can't start
		emit logLine(tr("Could not start java:"), MessageLevel::Error);
		emit logLines(result.errorLog.split('\n'), MessageLevel::Error);
		emit logLine("\nCheck your MultiMC Java settings.", MessageLevel::MultiMC);
		printSystemInfo(false, false);
		emitFailed(tr("Could not start java!"));
	}
	else
	{
		auto instance = m_parent->instance();
		printJavaInfo(result.javaVersion.toString(), result.mojangPlatform);
		instance->settings()->set("JavaVersion", result.javaVersion.toString());
		instance->settings()->set("JavaArchitecture", result.mojangPlatform);
		instance->settings()->set("JavaTimestamp", m_javaUnixTime);
		emitSucceeded();
	}
}

void CheckJava::printJavaInfo(const QString& version, const QString& architecture)
{
	emit logLine(tr("Java is version %1, using %2-bit architecture.\n\n").arg(version, architecture), MessageLevel::MultiMC);
	printSystemInfo(true, architecture == "64");
}

void CheckJava::printSystemInfo(bool javaIsKnown, bool javaIs64bit)
{
	auto cpu64 = Sys::isCPU64bit();
	auto system64 = Sys::isSystem64bit();
	if(cpu64 != system64)
	{
		emit logLine(tr("Your CPU architecture is not matching your system architecture. You might want to install a 64bit Operating System.\n\n"), MessageLevel::Error);
	}
	if(javaIsKnown)
	{
		if(javaIs64bit != system64)
		{
			emit logLine(tr("Your Java architecture is not matching your system architecture. You might want to install a 64bit Java version.\n\n"), MessageLevel::Error);
		}
	}
}
