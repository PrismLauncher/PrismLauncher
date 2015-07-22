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
#include <QStandardPaths>
#include <QFileInfo>

void CheckJava::executeTask()
{
	auto instance = m_parent->instance();
	auto javaPath = instance->settings()->get("JavaPath").toString();
	emit logLine("Java path is:\n" + m_javaPath + "\n\n", MessageLevel::MultiMC);

	auto realJavaPath = QStandardPaths::findExecutable(m_javaPath);
	if (realJavaPath.isEmpty())
	{
		emit logLine(tr("The java binary \"%1\" couldn't be found. You may have to set up java "
					"if Minecraft fails to launch.").arg(m_javaPath),
				 MessageLevel::Warning);
	}

	QFileInfo javaInfo(realJavaPath);
	qlonglong javaUnixTime = javaInfo.lastModified().toMSecsSinceEpoch();
	auto storedUnixTime = instance->settings()->get("JavaTimestamp").toLongLong();
	m_javaUnixTime = javaUnixTime;
	// if they are not the same, check!
	if(javaUnixTime != storedUnixTime)
	{
		m_JavaChecker = std::make_shared<JavaChecker>();
		bool successful = false;
		QString errorLog;
		QString version;
		emit logLine(tr("Checking Java version..."), MessageLevel::MultiMC);
		connect(m_JavaChecker.get(), &JavaChecker::checkFinished, this, &CheckJava::checkJavaFinished);
		m_JavaChecker->m_path = realJavaPath;
		m_JavaChecker->performCheck();
	}
	emitSucceeded();
}

void CheckJava::checkJavaFinished(JavaCheckResult result)
{
	if(!result.valid)
	{
		// Error message displayed if java can't start
		emit logLine(tr("Could not start java:"), MessageLevel::Error);
		emit logLines(result.errorLog.split('\n'), MessageLevel::Error);
		emit logLine("\nCheck your MultiMC Java settings.", MessageLevel::MultiMC);
		emitFailed(tr("Could not start java!"));
	}
	else
	{
		auto instance = m_parent->instance();
		emit logLine(tr("Java version is %1!\n").arg(result.javaVersion), MessageLevel::MultiMC);
		instance->settings()->set("JavaVersion", result.javaVersion);
		instance->settings()->set("JavaTimestamp", m_javaUnixTime);
		emitSucceeded();
	}
}
